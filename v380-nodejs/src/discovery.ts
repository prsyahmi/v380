import * as dgram from 'dgram';
const udp = dgram.createSocket('udp4');

export function discover() {
	let discoveryCount = 0;
	const cams: object = {};
	udp.bind(10009);

	udp.on('message', (msg, rinfo) => {
		const txt = msg.toString('utf-8');
		const parts = txt.split('^');

		if (parts.length < 16 || parts[0] !== 'NVDEVRESULT') {
			return;
		}

		if (!cams[parts[12]]) {
			cams[parts[12]] = true;
			console.log(parts);
		}
	});

	udp.on('listening', () => {
		udp.setBroadcast(true);
		udp.setRecvBufferSize(255);
		udp.setSendBufferSize(15);
		const discoverySend = () => {
			udp.send('NVDEVSEARCH^100', 10008);
		}

		console.log('Discovering...');
		discoverySend();
		const tmr = setInterval(() => {
			discoverySend();
			if (++discoveryCount === 5) {
				clearInterval(tmr);
				udp.close();
			}
		}, 1000);
	});

	udp.on('error', (err) => {
		console.error(err);
	});
}
