import paho.mqtt.client as mqtt
import json
from time import sleep  # Import sleep function

SLOT_DURATION = 0.02  # seconds

def on_connect(client, userdata, flags, rc):
    print("Connected with result code " + str(rc))
    client.subscribe("opentestbed/uinject/arrived")

def on_message(client, userdata, msg):
    data = json.loads(msg.payload.decode())  # Parse JSON data
    update_plot(data)

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

broker_address = "argus.paris.inria.fr"
broker_port = 1883

client.connect(broker_address, broker_port, 60)
client.loop_start()

# Initialize empty lists to store data
time_data = []
data_to_plot = {}

def update_plot(data):
    if isinstance(data, dict):  # Use isinstance for type checking
        src_id = data.get('src_id')
        avg_latency = data.get('avg_latency')

        if src_id is not None and avg_latency is not None:  # Ensure keys exist
            if src_id in data_to_plot:
                print(f"Updating data for src_id: {src_id}")
                data_to_plot[src_id].append(avg_latency * SLOT_DURATION)
            else:
                data_to_plot[src_id] = [avg_latency * SLOT_DURATION]
        else:
            print("Invalid data format: Missing 'src_id' or 'avg_latency'")

if __name__ == "__main__":
    while True:
        try:
            # Update the plot with new data
            sleep(0.1)  # Pause to allow the plot to update
        except KeyboardInterrupt:
            client.loop_stop()  # Stop the MQTT loop when exiting
            break

