const dgram = require('dgram');
const os = require('os');

const PORT = 5000;
const HOST = process.env.PL_LISTEN_ADDRESS || getLocalAddress();

function getLocalAddress() {
    const iface = Object.values(
        os.networkInterfaces()
    ).flat()
    .filter(iface => !iface.internal && iface.family === 'IPv4')[0];
    if (!iface) {
        throw Error('No iface to listen on!');
    }
    return iface.address;
}


const server = dgram.createSocket('udp4');

server.on('listening', () => {
    const address = server.address();
    console.log(`UDP Server listening on ${address.address}:${address.port}`);
});

let lastScrapeTime = Date.now();
server.on('message', (message, remote) => {
    const now = Date.now();
    console.log(message);
    const [checksum, temp, moist, photo] = message;
    const wifiAttempts = (message[4] >> 8) + message[5];
    const txnNumber = (message[6] >> 24) + (message[7] >> 16) + (message[8] >> 8) + message[9];
    const expectedCSum = ((temp*5) + (moist*2) + (photo*3) + wifiAttempts + txnNumber) % 255;
    if (checksum !== expectedCSum) {
        console.warn(`Incorrect checksum: expected ${expectedCSum}, got ${checksum}`);
        return;
    }
    const inteval = now - lastScrapeTime;
    lastScrapeTime = now;
    console.log(`${new Date().toString()} ${remote.address}:${remote.port} - txn: ${txnNumber} temp: ${temp}, moist: ${moist}, photo: ${photo} watmpt: ${wifiAttempts} (interval: ${inteval}ms)`);
});

server.bind(PORT, HOST);        