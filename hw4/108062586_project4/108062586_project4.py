
# -*- coding: UTF-8 -*-
from flask import Flask, request, abort
import DAN,csmapi, random, time, threading
from linebot import LineBotApi, WebhookHandler
from linebot.exceptions import InvalidSignatureError
from linebot.models import *
import requests, sys, os

# connect to IoTtalk server
# ServerURL = 'http://XXX.XXX.XX.XX:XXXX'
ServerURL = 'http://140.114.xxxx.xxxx:xxxx'
Reg_addr = None
# Note that Reg_addr 在以下三句會被換掉! # the mac_addr in DAN.py is NOT used
# mac_addr = 'CD8600D38' + str( random.randint(100,999 ) )
# 若希望每次執行這程式都被認為同一個 Dummy_Device, 要把上列 mac_addr 寫死, 不要用亂數。
# Reg_addr = mac_addr # Note that the mac_addr generated in DAN.py always be the same cause using UUID !

DAN.profile['dm_name']='108062586_1'   # you can change this but should also add the DM in server
DAN.profile['df_list']=['108062586_input', '108062586_output']   # Check IoTtalk to see what IDF/ODF the DM has
DAN.profile['d_name']= "project_D."+ str( random.randint(100,999 ) ) +"_"+ DAN.profile['dm_name'] # None
DAN.device_registration_with_retry(ServerURL, Reg_addr)

print("dm_name is ", DAN.profile['dm_name'])
print("Server is ", ServerURL)
# global gotInput, theInput, allDead    ## 主程式不必宣告 globel, 但寫了也 OK
gotInput = False
# theInput="haha"
theInput = None
check_result = None
# allDead = False

app = Flask(__name__)

# Channel Access Token
line_bot_api = LineBotApi('xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxFU=') 
# access token

# Channel Secret
handler = WebhookHandler('xxxxxxxxxxxxxxx') # channel screte

# 監聽所有來自 /callback 的 Post Request

@app.route("/callback", methods=['POST'])
def callback():
	# get X-Line-Signature header value
	signature = request.headers['X-Line-Signature']
	# get request body as text
	body = request.get_data(as_text=True)
	app.logger.info("Request body: " + body)
	# handle webhook body
	try:
		handler.handle(body, signature)
	except InvalidSignatureError:
		abort(400)
	return 'OK'

@handler.add(MessageEvent, message=TextMessage)
def handle_message(event):
	# print('aaa')
	response_message = None
	message = TextSendMessage(text=event.message.text)
	print("ready to push:" + message.text)

	if message.text.isnumeric():
		theInput = int(message.text)
		DAN.push ('108062586_input', theInput,  theInput)  #  試這:  DAN.push('device model', theInput)

		response_message = TextSendMessage(text = "push data: "+ str(theInput))
		# print("ready to push:" + response_message)
		try:
			# line_bot_api.reply_message(event.reply_token, response_message)
			gotInput = True   # notify my master that we have data
		except Exception as e:
			print(e)

		if gotInput:
			result = DAN.pull('108062586_output')
			print(str(result[0]))
			check_result = True

			response_message2 = None
			if check_result:
				print("got result")
				# response_message = TextSendMessage("got result")
				response_message2 = TextSendMessage(text = "pull data: "+ str(result[0]))
				# print("ready to push:" + response_message)
				# try:
				# 	# line_bot_api.reply_message(random.uniform(1, 8), response_message)
				# 	# line_bot_api.reply_message(event.reply_token, response_message)
				# except Exception as e:
				# 	print(e)		
		try:
			line_bot_api.reply_message(event.reply_token, [response_message, response_message2])
		except Exception as e:
			print(e)

	else:
		normal_msg = TextSendMessage(text = "normal data: " + message.text)
		if message.text == "terminate":
			try: 
				DAN.deregister()    # 試著解除註冊
				print("Bye ! --------------"+ DAN.profile['dm_name'], flush=True)				
				normal_msg = TextSendMessage(text = "terminte device: "+ DAN.profile['dm_name'] + " success.")
				# normal_msg = TextSendMessage(text = "terminte device: " + message.text)

			except Exception as e:
				print(e)

		# normal_msg = TextSendMessage(text = "normal data: " + message.text)
		line_bot_api.reply_message(event.reply_token, normal_msg)
		# line_bot_api.reply_message(event.reply_token, "haha")
	

	# line_bot_api.reply_message(event.reply_token, message)

	# you can write some codes here to handle the message sent by users

if __name__ == "__main__":

	# threadx = threading.Thread(target=doRead)
	# threadx.daemon = True
	# threadx.start()

	# while True:
	# 	try:
	# 		# if(allDead): break
	# 	#Pull data from a device feature called "Dummy_Control"
	# 		# value1 = DAN.pull('Dummy_Control')
	# 		# if value1 != None:
	# 		# 	print (value1[0])
	# 	#Push data to a device feature called "Dummy_Sensor" 
	# 		#value2=random.uniform(1, 10)    ## original Dummy_Device example

	# 		if gotInput:
	# 			# we have data in theInput
	# 			try:
	# 				value2 = float( theInput )
	# 				print("value2:" + vaule2)
	# 			except:
	# 				value2 = 0
	# 			# if(allDead): 
	# 			# 	break
	# 			# gotInput = False   # so that you can input again 
	# 			DAN.push ('108062586_1', value2,  value2)  #  試這:  DAN.push('Dummy_Sensor', theInput)

	# 	except Exception as e:
	# 		print(e)
	# 		if str(e).find('mac_addr not found:') != -1:
	# 			print('Reg_addr is not found. Try to re-register...')
	# 			DAN.device_registration_with_retry(ServerURL, Reg_addr)
	# 		else:
	# 			print('Connection failed due to unknow reasons.')
	# 			time.sleep(1)  
	# 	# try:
	# 	# 	time.sleep(0.2)
	# 	# except KeyboardInterrupt:
	# 	# 	break
	# time.sleep(0.25)

	# try: 
	# 	DAN.deregister()    # 試著解除註冊
	# 	print("Bye ! --------------"+ DAN.profile['dm_name'], flush=True)
	# except Exception as e:
	# 	print("===")
	
	# sys.exit( )




#------------------------------------------- origin
# 	# connect to IoTtalk server
# 	# ServerURL = 'http://XXX.XXX.XX.XX:XXXX'
	# Reg_addr = None


# # 	# Define your IoTtalk Device
# # 	# Hint: DAN.profile
# 	DAN.profile['dm_name']='Bulb'
# 	DAN.profile['df_list']=['Luminance']

# # 	# Register
# 	DAN.device_registration_with_retry(ServerURL, Reg_addr)
# 	try:
# 		IDF_data = random.uniform(1, 10)
# 		DAN.push('Status', int(IDF_data))
# 		ODF_data = DAN.pull('Name-O')
# 		if ODF_data != None:
# 			print(ODF_data[0])
# 	except Exception as e:
# 		print(e)
# 		if str(e).find('mac_addr not found:')!=-1:
# 			print('Reg_addr is not found. Try to re-register...')
# 			DAN.device_registration_with_retry(ServerURL, Reg_addr)
# 		else:
# 			print('Connection failed due to unknown reasons.')
# 			time.sleep(1)

	# Deregister
	# DAN.deregister()
	# exit()

	port = int(os.environ.get('PORT', 5000))
	app.run(host='0.0.0.0', port=port)
