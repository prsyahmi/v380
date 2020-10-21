import { argv } from 'process';
import * as crypto from 'crypto';
import * as discovery from './discovery';
import * as t from 'typebase';
import * as net from 'net';
import * as fs from 'fs';
import { promisify } from 'util';
import { handleStream, init } from './stream';

const readFileAsync = promisify(fs.readFile);
const statAsync = promisify(fs.stat);

const loginStruct = t.Struct.define([
	['command', t.i32],
	['unknown1', t.ui32], // 120 = LoginFromServerEX, 1002 = LoginFromMRServerEX, 1022 = our
	['unknown2', t.ui8], // 1, 2=our
	['unknown3', t.ui32],
	['deviceId', t.ui32], // 10 = LoginFromServerEX, 11 = LoginFromMRServerEX, 1=our
	['hostDateTime', t.List.define(t.ui8, 32)], // optional - maybe related to unk1/2, on v380 pc this is a domain name
	['username', t.List.define(t.ui8, 32)],
	['password', t.List.define(t.ui8, 32)], // consists of randomkey:password
]);

const streamLoginLanStruct = t.Struct.define([
	['command', t.i32],
	['deviceId', t.ui32],   // 4:  +292
	['unknown1', t.ui32],   // 8:  +284    = HSPI_V_StartPreview a4
	['maybeFps', t.ui16],   // 12: hardcoded to 20
	['authTicket', t.ui32], // 14: +304
	['unknown3', t.ui32],   // 18: unused
	['unknown4', t.ui32],   // 22: *(+344) == !0 + 4096 (v14 + 4096) "0x1001" audio related?
	['unknown5', t.ui32],   // 26: +288    = sound related?
	['unknown6', t.ui32],   // 30: unused
]);

const streamLoginCloudStruct = t.Struct.define([
	['command', t.i32],
	['unknown1', t.ui32],   // 1022
	['domain', t.List.define(t.ui8, 32)],
	['unknown2', t.List.define(t.ui8, 18)],
	['camPort', t.ui16],    // 58: v15 + 180 = 0x60 0x22 0x31 0x37 "8800"
	['unknown3', t.ui16],   // 60: This is not used (corrupted overlapped with domain/padding)
	['deviceId', t.ui32],   // 62: v51
	['authTicket', t.ui32], // 66: v49
	['session', t.ui32],    // 70: v52
	['unknown4', t.ui32],   // 74: v57 = 0
	['unknown5', t.ui8],    // 78: = 20 hardcoded
	['unknown6', t.ui32],   // 79: v14 = 0x1001
	['unknown7', t.ui32],   // 83: v56 = 0
]);

const streamStartStruct = t.Struct.define([
	['command', t.i32],
	['unknown1', t.ui32],
]);

const loginRespStruct = t.Struct.define([
	['command', t.i32],
	['loginResult', t.i32],
	['resultValue', t.i32],
	['version', t.ui8],
	['authTicket', t.ui32],
	['session', t.ui32],
	['deviceType', t.ui8],
	['camType', t.ui8],
	['vendorId', t.ui16],
	['isDomainExists', t.ui16],
	['domain', t.List.define(t.ui8, 32)],
	['recDevId', t.i32],
	['nChannels', t.ui8],
	['nAudioPri', t.ui8],
	['nVideoPri', t.ui8],
	['nSpeaker', t.ui8],
	['nPtzPri', t.ui8],
	['nReversePri', t.ui8],
	['nPtzXPri', t.ui8],
	['nPtzXCount', t.ui8],
	['settings', t.List.define(t.ui8, 32)],
	['panoX', t.ui16],
	['panoY', t.ui16],
	['panoRad', t.ui16], // 107
	['unknown1', t.ui32],
	['canUpdateDevice', t.ui32], // 112 (canUpdateDevice = [112] == 2)
]);

const streamLogin301RespStruct = t.Struct.define([
	['command', t.i32],
	['v21', t.i32],
	['maybeFps', t.ui16],
	['width', t.ui32],
	['height', t.ui32],
]);

interface IConfigFile {
	id: string;
	ip?: string;
	port?: number;
	username: string;
	password: string;
	serverPort: number;
}

if (0) {
	console.log(loginStruct, loginRespStruct, streamLoginCloudStruct, streamLoginLanStruct, streamLogin301RespStruct);
}

function generatePassword(password: string) {
	const generateRandomPrintable = (length: number) => {
		const set = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890!@#$%^&*()_+-=';
		let s = '';

		for (let i = 0; i < length; i++) {
			s += set[Math.floor(Math.random() * set.length)];
		}

		return s;
	}

	const nBlockLen = 16;
	const randomKey = generateRandomPrintable(nBlockLen);
	const staticKey = 'macrovideo+*#!^@';
	const pad = Buffer.alloc(nBlockLen - (password.length % nBlockLen), 0);

	const cipher1 = crypto.createCipheriv('aes-128-ecb', staticKey, null);
	const cipher2 = crypto.createCipheriv('aes-128-ecb', randomKey, null);

	cipher1.setAutoPadding(true);
	const catBuff = Buffer.concat([Buffer.from(password), pad]);

	const res1 = cipher1.update(catBuff);
	const res2 = cipher2.update(res1);

	return Buffer.concat([Buffer.from(randomKey), res2]);
}

async function entry() {
	if (argv.indexOf('discover') >= 0) {
		console.log('Discovering...');
		const res = discovery.discover();
		console.log(res);
		return;
	}

	try {
		await statAsync('./config.json');
	} catch (err) {
		console.error('Please create config.json file, see config.example.json');
		return;
	}

	try {
		const confRaw = await readFileAsync('./config.json');
		const conf: IConfigFile = JSON.parse(confRaw.toString());

		if (!conf.ip) {
			const discRes = await discovery.discover();
			console.log(discRes);
			// TODO: assign IP address after discovery
		}

		const host = conf.ip;
		const port = conf.port || 8800;
		const socketAuth = net.connect({
			host,
			port,
		});

		socketAuth.on('connect', () => {
			const buff = Buffer.alloc(256);
			const loginReq = new t.Pointer(buff, 0);

			const loginReqData = {
				command: 1167,
				deviceId: Buffer.from(conf.id),
				unknown1: 1022,
				unknown2: 2,
				unknown3: 1,
				hostDateTime: [],
				username: Buffer.from(conf.username),
				password: generatePassword(conf.password),
			}

			loginStruct.pack(loginReq, loginReqData);
			socketAuth.write(buff);
		});

		socketAuth.on('data', (data) => {
			const loginResp = loginRespStruct.unpack(new t.Pointer(data));

			if (loginResp.command === 1168) {
				if (loginResp.loginResult === 1001) {
					console.log('Camera logged in');
				} else if (loginResp.loginResult === 1011) {
					throw new Error('Invalid username');
				} else if (loginResp.loginResult === 1012) {
					throw new Error('Invalid password');
				} else if (loginResp.loginResult === 1018) {
					throw new Error('Invalid device id');
				}
			}

			socketAuth.destroy();
			startStreaming(conf, loginResp);
		});
	} catch (err) {
		console.error(err);
	}
}

entry().catch((err) => {
	console.error(err);
})

function startStreaming(conf: IConfigFile, resp: any) {
	let stage = 0;
	const buff = Buffer.alloc(256);
	const socketStream = net.connect({
		host: conf.ip,
		port: conf.port || 8800,
	});

	socketStream.setTimeout(5000);
	socketStream.setNoDelay(true); // Disable nagle algorithm

	socketStream.on('connect', () => {
		const streamLoginLanReq = new t.Pointer(buff, 0);

		const streamLoginLanData = {
			command: 301,
			deviceId: Buffer.from(conf.id),
			unknown1: 0,
			maybeFps: 20, // hardcoded in HSPC_PreviewDLL.dll, maybe fps?
			authTicket: resp.authTicket,
			unknown4: 4096 + 1, // not sure (maybe audio buffer)
			unknown5: 0,
		}

		streamLoginLanStruct.pack(streamLoginLanReq, streamLoginLanData);
		socketStream.write(buff);
	});

	socketStream.on('readable', () => {
		if (stage === 0) {
			let data = null;

			data = socketStream.read(socketStream.readableLength);
			if (data === null) {
				return;
			}

			const streamLoginResp = streamLogin301RespStruct.unpack(new t.Pointer(data))
			if (streamLoginResp.command !== 401) {
				throw new Error(`Login response: expected 401, got ${streamLoginResp.command}`);
			}

			if (streamLoginResp.v21 === -11 || streamLoginResp.v21 === -12) {
				console.error(`Login response: unsupported ${streamLoginResp.v21}, continuing`);
			} else if (streamLoginResp.v21 !== 402 && streamLoginResp.v21 !== 1001) {
				throw new Error(`Login response: unsupported ${streamLoginResp.v21}`);
			}

			const streamStartReq = new t.Pointer(buff, 0);
			const streamStartData = {
				command: 303,
				unknown1: streamLoginResp.v21,
			}

			streamStartStruct.pack(streamStartReq, streamStartData);
			socketStream.write(buff);

			stage++;
			init(conf.serverPort, streamLoginResp);
			readStreamPacket(socketStream);
		} else if (stage === 1) {
			// readStreamPacket(socketStream);
		}
	});

	socketStream.on('end', (hadErr) => {
		console.log('Stream socket closed', hadErr);
	})

	socketStream.on('error', (err) => {
		console.error(err);
	});
}

async function readStreamPacket(socketStream: net.Socket) {
	const data = socketStream.read(12);
	if (data !== null) {
		if (data[0] === 0x7f) {
			await handleStream(data, socketStream);
		} else if (data[0] === 0x1f) {
			console.error('Unparsed 0x1f data');
		} else if (data[0] === 0x6f) {
			// Should sleep(20 ms)
		} else {
			console.error('Unknown data: ' + data[0]);
		}
	}

	setTimeout(() => {
		readStreamPacket(socketStream);
	}, 0);
}
