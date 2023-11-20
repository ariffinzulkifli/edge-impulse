import os
import csv
import random
import string
from flask import Flask, request, jsonify

def generate_filename(label, directory):
    ''' Generate a filename with a random 12-character string in the specified directory. '''
    random_string = ''.join(random.choices(string.ascii_lowercase + string.digits, k=12))
    return os.path.join(directory, f'{label}.{random_string}.csv')

# Define the directory to store CSV files
data_directory = 'data'

# Create the directory if it does not exist
if not os.path.exists(data_directory):
    os.makedirs(data_directory)

app = Flask(__name__)

# Global variable for label
label = ''

@app.route('/hello', methods=['GET'])
def hello():
    # Simple endpoint for ESP32 to check connectivity
    return jsonify({'message': 'Holla, send the data to me now.'})

@app.route('/data-catcher', methods=['POST'])
def collect_data():
    global label
    data = request.json
    samples = data['data']

    csv_file_name = generate_filename(label, data_directory)

    # Check if file exists to determine if we need to write the header
    file_exists = os.path.isfile(csv_file_name)

    with open(csv_file_name, 'a', newline='') as file:
        writer = csv.writer(file)
        # Write header if file does not exist
        if not file_exists:
            header = ['timestamp', 'accelX', 'accelY', 'accelZ', 'gyroX', 'gyroY', 'gyroZ']  # Modify as per your data columns
            writer.writerow(header)
        for sample in samples:
            writer.writerow(sample)

    return jsonify({'message': f'Data received and stored in {csv_file_name}'})

if __name__ == '__main__':
    label = input('Enter the label name for this data: ')
    app.run(debug=True, host='0.0.0.0', port=5050, use_reloader=False)
