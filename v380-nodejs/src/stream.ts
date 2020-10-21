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

let width = 0;
let height = 0;

export function init(packet: any) {
	console.log(packet);
	width = packet.width;
	height = packet.height;
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

const vidStream = new stream.PassThrough();
const audStream = new stream.PassThrough();

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
				vidStream.write(Buffer.concat(videoPacketQueue));
				videoPacketQueue = [];
			}
			break;

		case 0x16:
			// Audio
			audioPacketQueue.push(streamData);
			if (streamPacket.curFrame === streamPacket.totalFrame - 1) {
				audStream.write(Buffer.concat(audioPacketQueue));
				audioPacketQueue = [];
			}
			break;
	}
}

const app = express();

app.get('/video/:filename', (req, res) => {
	res.contentType('flv');
	// make sure you set the correct path to your video file storage
	// const pathToMovie = __dirname + '/../' + req.params.filename;
	ffmpeg()
		// input
		.input(vidStream)
		.inputFPS(20)
		// output
		.format('flv')
		.flvmeta()
		.size('1920x1080')
		//.videoBitrate('512k')
		//.videoCodec('libx264')
		.fps(20)
		.audioBitrate('96k')
		.audioCodec('aac')
		.audioFrequency(22050)
		.audioChannels(2)

		//.addOption(['-probesize 32', '-formatprobesize 0', '-avioflags direct', '-flags low_delay'])
		//.input(audStream)
		// use the 'flashvideo' preset (located in /lib/presets/flashvideo.js)
		.on('end', () => {
			console.log('file has been converted succesfully');
		})
		.on('error', (err) => {
			console.log('an error happened: ' + err.message);
		})
		//.mergeAdd(audStream)
		.pipe(res, { end: true });
});

app.listen(4000);
