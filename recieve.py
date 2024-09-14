from flask import Flask, request, jsonify
import json
import os
import logging
from logging.handlers import RotatingFileHandler

app = Flask(__name__)

# Set up logging
logging.basicConfig(level=logging.DEBUG)
handler = RotatingFileHandler('flask_server.log', maxBytes=10000, backupCount=1)
handler.setLevel(logging.DEBUG)
app.logger.addHandler(handler)

FILE_PATH = 'C:\\Users\\Kais Bouthouri\\Desktop\\received_data.json'

@app.route('/<device_id>', methods=['POST'])
def receive_data(device_id):
    app.logger.info(f"Received a request to /{device_id}")
    app.logger.debug(f"Request headers: {request.headers}")
    app.logger.debug(f"Request data: {request.data}")
    
    # Validate device_id to ensure it's within the expected range
    if device_id not in [f'ESP32_{i}' for i in range(1, 7)]:
        app.logger.warning(f"Invalid device_id: {device_id}")
        return jsonify({'status': 'error', 'message': 'Invalid device ID'}), 400

    try:
        data = request.json
        app.logger.info(f"Parsed JSON data: {data}")
        
        if not data or not isinstance(data, dict):
            app.logger.warning("Received invalid data")
            return jsonify({'status': 'error', 'message': 'Invalid data'}), 400
        
        if not os.path.exists(FILE_PATH):
            existing_data = []
            app.logger.info(f"Created new file at {FILE_PATH}")
        else:
            with open(FILE_PATH, 'r') as f:
                try:
                    existing_data = json.load(f)
                    app.logger.info(f"Loaded existing data from {FILE_PATH}")
                except json.JSONDecodeError:
                    existing_data = []
                    app.logger.warning(f"JSON decode error when reading {FILE_PATH}. Starting with empty list.")
        
        # Add device_id to the received data
        data['device_id'] = device_id
        existing_data.append(data)
        
        with open(FILE_PATH, 'w') as f:
            json.dump(existing_data, f, indent=4)
            app.logger.info(f"Saved updated data to {FILE_PATH}")
        
        return jsonify({'status': 'success', 'message': 'Data received and saved'}), 200
    except Exception as e:
        app.logger.error(f"Error in receive_data: {str(e)}", exc_info=True)
        return jsonify({'status': 'error', 'message': str(e)}), 500

@app.route('/test', methods=['GET'])
def test():
    app.logger.info("Received a request to /test")
    return "Server is running!"

if __name__ == '__main__':
    app.logger.info("Starting Flask server...")
    app.run(host='143.93.153.168', port=5100, debug=True)
