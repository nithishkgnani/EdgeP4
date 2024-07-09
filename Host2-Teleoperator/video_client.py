#!/home/zenlab/Documents/temp/venv/bin/python python3
# This is client code to receive video frames over UDP
import cv2, imutils, socket, csv
import numpy as np
import time, random
import base64

BUFF_SIZE = 18000
client_socket = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
client_socket.setsockopt(socket.SOL_SOCKET,socket.SO_RCVBUF,BUFF_SIZE)
# client_socket.setSendBufferSize(400)
host_name = socket.gethostname()
host_ip = '10.0.0.1' 
port = 5001
print(host_ip, port)
# random number headers for duplicate identification
temp1=random.randint(0,65535)
temp2=random.randint(0,65535)
message =temp1.to_bytes(2,'big')+temp2.to_bytes(2,'big')+ b'Hello'
meta_list=[]
client_socket.sendto(message,(host_ip,port))
vid = cv2.VideoCapture(0) #  replace 'rocket.mp4' with 0 for webcam
fps,st,frames_to_count,cnt = (0,0,20,0)
while True:
	msg,server_addr=client_socket.recvfrom(BUFF_SIZE)
	print('GOT connection from ',server_addr)
	WIDTH=600  # size of frame
	while(vid.isOpened()):
		try:
			_,frame = vid.read()
			temp1=random.randint(0,65535)
			temp2=random.randint(0,65535)
			frame = imutils.resize(frame,width=WIDTH)
			encoded,buffer = cv2.imencode('.jpg',frame,[cv2.IMWRITE_JPEG_QUALITY,30])
			message = temp1.to_bytes(2,'big')+temp2.to_bytes(2,'big')+base64.b64encode(buffer)
			meta_list.append(message[0:4])
			client_socket.sendto(message,server_addr)
			frame = cv2.putText(frame,'FPS: '+str(fps),(10,40),cv2.FONT_HERSHEY_SIMPLEX,0.7,(0,0,255),2)
			cv2.imshow('TRANSMITTING VIDEO',frame)
			key = cv2.waitKey(1) & 0xFF
			if key == ord('q'):
				client_socket.close()
				break
			if cnt == frames_to_count:
				try:
					fps = round(frames_to_count/(time.time()-st))
					st=time.time()
					cnt=0
				except:
					pass
			cnt+=1
			# time.sleep(0.06)
		except KeyboardInterrupt:
			# file= "./Data_and_Plots/Result_CBtest_015_UR3_vidLog.csv"
			# file = "./Data_and_Plots/Result_DemoTest01.csv"
			# with open(file, 'w') as f:
			# 	# using csv.writer method from CSV package
			# 	write = csv.writer(f)
			# 	write.writerows(meta_list)
			exit()

