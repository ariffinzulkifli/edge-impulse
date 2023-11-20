import requests
from flask import Flask, request, jsonify

TELEGRAM_BOT_TOKEN = '5780940485:AAEt7n8SWZkO7AeO9GrTjd__Q2Zgzsl_dmA'  # Replace with your Telegram bot token
TELEGRAM_CHAT_ID = ''  # This will be set by get_chat_id()

def send_telegram_message(message):
    '''Send a message to a Telegram chat.'''
    if not TELEGRAM_CHAT_ID:
        return {'error': 'Chat ID is not set.'}

    url = f'https://api.telegram.org/bot{TELEGRAM_BOT_TOKEN}/sendMessage'
    data = {
        'chat_id': TELEGRAM_CHAT_ID,
        'text': message
    }
    response = requests.post(url, data=data)
    return response.json()

def get_chat_id():
    global TELEGRAM_CHAT_ID  # Declare the global variable
    url = f'https://api.telegram.org/bot{TELEGRAM_BOT_TOKEN}/getUpdates'
    response = requests.get(url)
    updates = response.json()

    if updates['ok'] and updates['result']:
        for update in updates['result']:
            if 'message' in update and 'chat' in update['message']:
                TELEGRAM_CHAT_ID = update['message']['chat']['id']
                print(f'Chat ID: {TELEGRAM_CHAT_ID}')
                break  # Break after getting the first chat ID
    else:
        print('No updates available or failed to retrieve updates.')

app = Flask(__name__)

@app.route('/send-notification', methods=['POST'])
def send_notification():
    data = request.json
    message = data.get('message', 'No message provided')
    result = send_telegram_message(message)
    return jsonify(result)

if __name__ == '__main__':
    get_chat_id()
    app.run(debug=True, host='0.0.0.0', port=5050, use_reloader=False)
