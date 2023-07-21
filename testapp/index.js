const HID = require(`node-hid`);
const devices = HID.devices();


// .idVendor = 0x1231,
// .idProduct = 0x3321,
const foundDev = devices.filter(v => {
	return v.vendorId === 0x1231 && v.productId === 0x3321;
});

(async () => {
	if (!foundDev[0]) {
		return;
	}
	const testDevice = new HID.HID(foundDev[0].path);

	testDevice.on('data', (data) => {
		console.log(data);
	});

	setInterval(() => {
		testDevice.write([0x01, 0x01, 0x01, 0x05, 0xff, 0xff]);
	}, 500);
	console.log(foundDev);
})();
