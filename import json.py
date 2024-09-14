import json
from collections import defaultdict
import os

def analyze_rssi(file_path, target_mac):
    try:
        with open(file_path, 'r') as file:
            data_list = json.load(file)
        
        results = {}
        
        for data in data_list:
            device_id = data['device_id']
            rssi_sum = 0
            rssi_count = 0
            
            for packet in data['packets']:
                if packet['src_mac'] == target_mac:
                    rssi_sum += packet['rssi']
                    rssi_count += 1
            
            if rssi_count > 0:
                avg_rssi = rssi_sum / rssi_count
                results[device_id] = avg_rssi
        
        if results:
            for device_id, avg_rssi in results.items():
                print(f"* **{device_id}**: average RSSI = {avg_rssi:.2f}")
        else:
            print(f"No data found for MAC address {target_mac}")
        
        return results  # Still return results for potential use in other functions
    except FileNotFoundError:
        print(f"Error: File not found at {file_path}")
        return {}
    except json.JSONDecodeError:
        print(f"Error: Invalid JSON in file {file_path}")
        return {}
    except KeyError as e:
        print(f"Error: Missing key in JSON structure: {e}")
        return {}
    except Exception as e:
        print(f"An unexpected error occurred: {e}")
        return {}

def main():
    target_mac = "E0:46:EE:53:51:21"  # Replace with your target MAC address
    file_path = r"C:\Users\Kais Bouthouri\Desktop\received_data.json"
    file_path = os.path.normpath(file_path)  # Normalize the path for the current OS
    
    analyze_rssi(file_path, target_mac)

if __name__ == "__main__":
    main()