
# This is server code to send video frames over UDP
import cv2, imutils, socket
import numpy as np
import time, random
import base64

BUFF_SIZE = 18000
server_socket = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
server_socket.setsockopt(socket.SOL_SOCKET,socket.SO_RCVBUF,BUFF_SIZE)
host_name = socket.gethostname()
host_ip = '10.0.0.1'
print(host_ip)
port = 5001
socket_address = (host_ip,port)
server_socket.bind(socket_address)
print('Listening at:',socket_address)
fps,st,frames_to_count,cnt = (0,0,20,0)
temp1=random.randint(0,65535)
temp2=random.randint(0,65535)
message = temp1.to_bytes(2,'big')+temp2.to_bytes(2,'big')+b'hello'

# server_socket.sendto(message)
while True:
	msg,client_addr=server_socket.recvfrom(BUFF_SIZE)
	if msg[4:]==b'Hello':
		server_socket.sendto(message,client_addr)
		break

while True:
	packet,_ = server_socket.recvfrom(BUFF_SIZE)
	WIDTH=600  # size of frame
	try:
		# accepting frame after stripping off random number headers
		data = base64.b64decode(packet[4:],' /')
		npdata = np.fromstring(data,dtype=np.uint8)
		frame = cv2.imdecode(npdata,1)
		frame = imutils.resize(frame,width=WIDTH) # resize the frame
		frame = cv2.putText(frame,'FPS: '+str(fps),(10,40),cv2.FONT_HERSHEY_SIMPLEX,0.7,(0,0,255),2)
		cv2.imshow("RECEIVING VIDEO",frame)
		key = cv2.waitKey(1) & 0xFF
		if key == ord('q'):
			server_socket.close()
			break
		if cnt == frames_to_count:
			try:
				fps = round(frames_to_count/(time.time()-st))
				st=time.time()
				cnt=0
			except:
				pass
		cnt+=1
	except:
		data=b''


