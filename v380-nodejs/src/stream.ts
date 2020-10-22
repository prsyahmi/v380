// import beamcoder = require('beamcoder');
import * as ffmpeg from 'fluent-ffmpeg';
import * as express from 'express';
import { Socket } from 'net';
import * as t from 'typebase';
import * as stream from 'stream';
// import * as fs from 'fs';

interface IStreamInfo {
	id: number;
	stream: stream.PassThrough;
	hasIframe: boolean;
	_coll: IStreamInfo[];
}

const streamStruct = t.Struct.define([
	['type', t.ui8],
	['index', t.ui8],
	['totalFrame', t.ui16],
	['curFrame', t.ui16],
	['len', t.ui16],
]);

let curId = 0;
let width = 0;
let height = 0;
let fps = 0;
let serverPort = 0;
const app = express();

// Workaround, need to rewrite to start stream when first client connected
const TOTAL_PFRAME: number = 20;
let pFrame: Buffer[] = [];
let iFrame: Buffer;

export function init(listenPort: number, packet: any) {
	width = packet.width;
	height = packet.height;
	fps = packet.maybeFps;
	serverPort = listenPort;
	console.log(`${width}x${height} @ ${fps} fps`);

	app.listen(listenPort);
	console.log(`Server started at port ${serverPort}`);
}

function streamChunk(sock: Socket, size: number): Promise<Buffer> {
	return new Promise((resolve, reject) => {
		const getChunk = () => {
			const data = sock.read(size);
			if (data !== null) {
				return resolve(data);
			}
			setTimeout(getChunk, 0);
		};

		getChunk();
	});
}

const vidStreams: IStreamInfo[] = [];
const audStreams: IStreamInfo[] = [];

function createStream(collections: IStreamInfo[]) {
	const id = ++curId;
	const strm = new stream.PassThrough();
	(strm as any)._id = id;

	const info: IStreamInfo = {
		hasIframe: false,
		id,
		stream: strm,
		_coll: collections,
	};

	collections.push(info);
	return info;
}

function removeStream(strm: IStreamInfo) {
	const i = strm._coll.findIndex((v: any) => v._id === strm.id);
	if (i >= 0) {
		strm._coll.splice(i, 1);
	}
}

let videoPacketQueue: Buffer[] = [];
let audioPacketQueue: Buffer[] = [];
export async function handleStream(packet: Buffer, sock: Socket) {
	const streamPacket = streamStruct.unpack(new t.Pointer(packet, 1));

	if (streamPacket.len > 500 || !streamPacket.totalFrame || streamPacket.curFrame > streamPacket.totalFrame) {
		console.error('Sanity check failed, bailing out');
		return;
	}
	const streamData = await streamChunk(sock, streamPacket.len);

	switch (streamPacket.type) {
		case 0x00: // Probably I-frame
			videoPacketQueue.push(streamData);
			if (streamPacket.curFrame === streamPacket.totalFrame - 1) {
				// console.log('i frame');
				const concatBuff = Buffer.concat(videoPacketQueue);
				iFrame = concatBuff;
				pFrame = [];
				for (const s of vidStreams) {
					s.stream.write(concatBuff);
				}
				videoPacketQueue = [];
			}
			break;
		case 0x01: // Probably P-frame
			// Video
			videoPacketQueue.push(streamData);
			if (streamPacket.curFrame === streamPacket.totalFrame - 1) {
				// console.log('p frame');
				const concatBuff = Buffer.concat(videoPacketQueue);
				if (pFrame.length <= TOTAL_PFRAME) {
					pFrame.push(concatBuff);
				}

				for (const s of vidStreams) {
					if (s.hasIframe) {
						s.stream.write(concatBuff);
					}
				}
				videoPacketQueue = [];
			}
			break;

		case 0x16:
			// Audio
			audioPacketQueue.push(streamData);
			if (streamPacket.curFrame === streamPacket.totalFrame - 1) {
				const concatBuff = Buffer.concat(audioPacketQueue).slice(17);
				for (const s of audStreams) {
					s.stream.write(concatBuff);
				}
				audioPacketQueue = [];
			}
			break;

		default:
			console.log('unknown type', streamPacket.type);
	}
}

app.get('/audio/:filename', (req, res) => {
	const newStream = createStream(audStreams);

	res.contentType('flv');
	const f = ffmpeg()
		.format('flv')
		.input(newStream.stream)
		.withInputOption(['-f s16le', '-ar 8000', '-acodec adpcm_ima_ws', '-ac 1'])
		.noVideo()
		.audioBitrate('32k')
		.audioCodec('aac')
		.audioFrequency(8000)
		.audioChannels(1)

	f
		.on('end', () => {
			removeStream(newStream);
		})
		.on('error', (err) => {
			console.log(err.message);
			removeStream(newStream);
		})
		.pipe(res, { end: true });

	// console.log(f._getArguments());
});

app.get('/video/:filename', (req, res) => {
	const newStream = createStream(vidStreams);
	if (iFrame) {
		newStream.stream.write(iFrame);
		for (const frame of pFrame) {
			newStream.stream.write(frame);
		}
	}
	newStream.hasIframe = true;

	res.contentType('flv');
	const f = ffmpeg()
		// input
		.input(newStream.stream)
		.withInputOption(['-vcodec h264', '-probesize 32', '-formatprobesize 0', '-avioflags direct', '-flags low_delay'])
		.withInputFPS(fps)
		.input(`http://localhost:${serverPort}/audio/stream.flv`)
		// output
		.format('flv')
		.flvmeta()
		// .size(`${width}x${height}`)
		// .videoBitrate('512k')
		// .videoCodec('libx264')
		.videoCodec('copy')
		.fps(fps)
		// .noAudio()
		.audioCodec('copy');

	f
		.on('end', () => {
			removeStream(newStream);
			console.log(`Stream ${newStream.id} stopped`);
		})
		.on('error', (err) => {
			console.log(err.message);
			removeStream(newStream);
			console.log(`Stream ${newStream.id} stopped with error`);
		})
		.pipe(res, { end: true });

	console.log(`Stream ${newStream.id} started`);
	console.log(f._getArguments());
});
