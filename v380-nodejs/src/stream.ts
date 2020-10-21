// import beamcoder = require('beamcoder');
import * as ffmpeg from 'fluent-ffmpeg';
import * as express from 'express';
import { Socket } from 'net';
import * as t from 'typebase';
import * as stream from 'stream';
// import * as fs from 'fs';

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
const app = express();

export function init(serverPort: number, packet: any) {
	width = packet.width;
	height = packet.height;
	fps = packet.maybeFps;
	console.log(`${width}x${height} @ ${fps} fps`);

	app.listen(serverPort);
	console.log(`Server started at port ${serverPort}`);
}

function streamChunk(sock: Socket, size: number): Promise<Buffer> {
	return new Promise((resolve, reject) => {
		const getChunk = () => {
			const data = sock.read(size);
			if (data !== null) {
				return resolve(data);
			}
			setImmediate(getChunk);
		};

		getChunk();
	});
}

const vidStreams: stream.PassThrough[] = [];
const audStreams: stream.PassThrough[] = [];

function createStream(collections: stream.PassThrough[]) {
	const id = ++curId;
	const strm = new stream.PassThrough();
	(strm as any)._id = id;
	collections.push(strm);

	return {
		id,
		strm
	};
}

function removeStream(collections: stream.PassThrough[], id: number) {
	const i = collections.findIndex((v: any) => v._id === id);
	if (i >= 0) {
		collections.splice(i, 1);
	}
}

///const audFile = fs.createWriteStream('test2.wav');

let videoPacketQueue: Buffer[] = [];
let audioPacketQueue: Buffer[] = [];
export async function handleStream(packet: Buffer, sock: Socket) {
	const streamPacket = streamStruct.unpack(new t.Pointer(packet, 1));

	if (streamPacket.len > 500 || !streamPacket.totalFrame || streamPacket.curFrame > streamPacket.totalFrame) {
		console.error('Sanity check failed, bailing out');
		return;
	}
	const streamData = await streamChunk(sock, streamPacket.len);

	if (packet.length === 1 && width && height) {
		console.log(streamData);
	}

	switch (streamPacket.type) {
		case 0x00:
		case 0x01:
			// Video
			videoPacketQueue.push(streamData);
			if (streamPacket.curFrame === streamPacket.totalFrame - 1) {
				const concatBuff = Buffer.concat(videoPacketQueue);
				for (const s of vidStreams) {
					s.write(concatBuff);
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
					s.write(concatBuff);
				}
				//const audFile = fs.createWriteStream('test' + streamPacket.index + '.wav');
				//audFile.write(Buffer.concat(audioPacketQueue));
				//audFile.end();
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
	ffmpeg()
		.format('flv')
		.input(newStream.strm)
		.withInputOption(['-f s16le', '-ar 8000', '-acodec adpcm_ima_ws', '-ac 1'])
		.noVideo()
		.audioBitrate('32k')
		.audioCodec('aac')
		.audioFrequency(8000)
		.audioChannels(1)

		.on('end', () => {
			removeStream(audStreams, newStream.id);
		})
		.on('error', (err) => {
			console.log(err.message);
			removeStream(audStreams, newStream.id);
		})
		.pipe(res, { end: true });
});

app.get('/video/:filename', (req, res) => {
	const newStream = createStream(vidStreams);

	res.contentType('flv');
	const f = ffmpeg()
		// input
		.input(newStream.strm)
		.inputFPS(20)
		//.input('http://localhost:4000/audio/test.flv')
		//.addOption(['-vsync 0'])
		// output
		.format('flv')
		.flvmeta()
		//.size('1920x1080')
		//.videoBitrate('512k')
		//.videoCodec('libx264')
		.videoCodec('copy')
		.fps(20)
		.noAudio()
	//.audioCodec('copy')
	//.audioBitrate('32k')
	//.audioCodec('aac')
	//.audioFrequency(8000)
	//.audioChannels(1)

	f
		.on('end', () => {
			removeStream(vidStreams, newStream.id);
			console.log(`Stream ${newStream.id} stopped`);
		})
		.on('error', (err) => {
			console.log(err.message);
			removeStream(vidStreams, newStream.id);
			console.log(`Stream ${newStream.id} stopped with error`);
		})
		.pipe(res, { end: true });

	console.log(`Stream ${newStream.id} started`);
	console.log(f._getArguments());
});
