const dgram = require('dgram');
const os = require('os');

const PORT = 5000;
const HOST = getLocalAddress();

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

server.on('message', (message, remote) => {
    console.log(`${remote.address}:${remote.port} - ${message}`);
});

server.bind(PORT, HOST);