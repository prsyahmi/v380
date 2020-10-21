import * as dgram from 'dgram';
const udp = dgram.createSocket('udp4');

export async function discover() {
	return new Promise((resolve, reject) => {
		let discoveryCount = 0;
		const cams: object = {};
		udp.bind(10009);
		const camData: any[] = [];

		udp.on('message', (msg, rinfo) => {
			const txt = msg.toString('utf-8');
			const parts = txt.split('^');

			if (parts.length < 16 || parts[0] !== 'NVDEVRESULT') {
				return;
			}

			if (!cams[parts[12]]) {
				cams[parts[12]] = true;
				camData.push(parts);
			}
		});

		udp.on('listening', () => {
			udp.setBroadcast(true);
			udp.setRecvBufferSize(255);
			udp.setSendBufferSize(15);
			const discoverySend = () => {
				udp.send('NVDEVSEARCH^100', 10008);
			}

			discoverySend();
			const tmr = setInterval(() => {
				discoverySend();
				if (++discoveryCount === 5) {
					clearInterval(tmr);
					udp.close();
					resolve(camData);
				}
			}, 1000);
		});

		udp.on('error', (err) => {
			reject(err);
		});
	});
}
