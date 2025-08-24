import usb.core
import usb.util
import struct
import time
import csv
from datetime import datetime

# DS4 Vendor/Product ID
VENDOR_ID  = 0x054c
PRODUCT_ID = 0x09cc

# 1. Find the DualShock4 controller
dev = usb.core.find(idVendor=VENDOR_ID, idProduct=PRODUCT_ID)
if dev is None:
    raise IOError("DualShock4 not found. Make sure it's connected.")

# 2. Detach the kernel driver if it's active
if dev.is_kernel_driver_active(0):
    dev.detach_kernel_driver(0)

# 3. Set the configuration
dev.set_configuration()
cfg  = dev.get_active_configuration()
intf = cfg[(0, 0)]
ep   = usb.util.find_descriptor(
    intf,
    custom_match=lambda e: usb.util.endpoint_direction(e.bEndpointAddress) == usb.util.ENDPOINT_IN
)
if ep is None:
    raise IOError("IN endpoint not found on interface 0.")

print("Reading DS4 sensors (Ctrl+C to stop)...")

records = []
start_time = time.monotonic()

try:
    while True:
        data = dev.read(ep.bEndpointAddress, ep.wMaxPacketSize, timeout=5000)
        now = time.monotonic()
        elapsed = now - start_time  # seconds as double

        # Gyro (bytes 13-20)
        gx = struct.unpack_from("<h", data, 13)[0]
        gy = struct.unpack_from("<h", data, 15)[0]
        gz = struct.unpack_from("<h", data, 17)[0]

        # Accelerometer (bytes 19-24)
        ax = struct.unpack_from("<h", data, 19)[0]
        ay = struct.unpack_from("<h", data, 21)[0]
        az = struct.unpack_from("<h", data, 23)[0]

        raw = data[30]
        cable = (raw >> 4) & 0x01
        lvl   = raw & 0x0F

        if not cable or lvl > 10:
            charging = False
        else:
            charging = True
        if not cable:
            lvl += 1
        if lvl > 10:
            lvl = 10
        battery_percent = lvl * 10  # 0–100%

        print(f"Battery: {battery_percent}% {'(charging)' if charging else ''}")

        print(f"[{elapsed:>10.6f} sec] Gx: {gx:>8} Gy: {gy:>8} Gz: {gz:>8} Ax: {ax:>8} Ay: {ay:>8} Az: {az:>8}")
        records.append((elapsed, gx, gy, gz, ax, ay, az))
        time.sleep(0.001)

except KeyboardInterrupt:
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"ds4_report_{ts}.csv"
    with open(filename, 'w', newline='') as f:
        writer = csv.writer(f, delimiter=' ')
        writer.writerow(['time', 'gyro_x', 'gyro_y', 'gyro_z', 'acc_x', 'acc_y', 'acc_z'])
        writer.writerows(records)
    print(f"\nSaved {len(records)} samples to {filename}")

finally:
    usb.util.dispose_resources(dev)
