import asyncio
from bleak import BleakScanner
from bleak import BleakClient
import struct
import time

class BLE_Read:

    def __init__(self):
        self.char_uuid = "89674523-01ef-cdab-8967-452301efcdab"
        self.time_prev = time.time()

    def callback(self, sender, data: bytearray):
        try:
            raw_values = bytearray(data)
            values = []
            for i in range(0, len(raw_values) - 1, 2):
                next = struct.unpack('<h', raw_values[i:i+2])[0]
                values.append(next)

            time_curr = time.time()
            print(f"values: {values}        time: {time_curr - self.time_prev}")
            self.time_prev = time_curr
        except Exception as e:
            print(f"Callback error: {e}")


    async def find_rc(self):
        address = "unknown"
        name = "RC"
        while address == "unknown":
            print("scanning...")
            devices = await BleakScanner.discover(timeout = 5.0)

            for d in devices:
                if d.name == name or d.name == "nimble":
                    address = d.address
                    print(f"RC found at {address} with name ``{d.name}``")
        
        client = BleakClient(address)
        connected = False
        while not connected:
            try:
                await client.connect()
                connected = True
            except Exception as e:
                print(f"Connection failed: {e}")
                print("Retrying...")
                await asyncio.sleep(2)
                
        print("Connected")
        print("Starting notify...")
        await client.start_notify(self.char_uuid, self.callback)

        try:
            while True:
                await asyncio.sleep(1)
        except KeyboardInterrupt:
            print("Keyboard Interrupt with ^C")
        finally:
            await client.stop_notify(self.char_uuid)
            print("Stopped notify...")
            await client.disconnect()
            print("Disconnected")

def main():
    ble = BLE_Read()
    asyncio.run(ble.find_rc())

if __name__ == "__main__":
    main()