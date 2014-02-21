# Echo client program
import socket
import sys

import array

#from matplotlib import pyplot as plt
#import numpy as np

max_buf_len = 10240

HOST = '10.1.1.2'    # The remote host
PORT = 9999          # The same port as used by the server
s = None
addr = None

def receiveImage():
    global s
    global addr
    global max_buf_len
    global HOST
    global PORT
    
    image = []
    tmpBuffer = None
    imgSize = 0
    recvBytes = 0
    recvEnd = False
    totalBytesReceived = 0
    recvStart = False
    tempAddr = None
    while not recvStart:
        while recvBytes <= 0:
            tmpBuffer,tempAddr = s.recvfrom(max_buf_len)
            recvBytes = len(tmpBuffer)
        msg = tmpBuffer.split(',')
        if msg[0] and msg[0] == 'START':
            recvStart = True
            imgSize = int(msg[1])
            print 'Ground Station: image size =', imgSize
            if imgSize <= 0:
                break
            while totalBytesReceived <= imgSize and not recvEnd:
                tmpBuffer,tempAddr = s.recvfrom(max_buf_len)
                recvBytes = len(tmpBuffer)
                totalBytesReceived = totalBytesReceived + recvBytes
                if 'END' in ''.join(tmpBuffer):
                    print 'Ground Station: image ended with: ',tmpBuffer
                    recvEnd = True
                else:
                    image.append(tmpBuffer)
    print 'Ground Station: received {} bytes'.format(totalBytesReceived)
    return image

def main():
    global s
    global addr
    global max_buf_len
    global HOST
    global PORT
    
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.sendto('Start Image', (HOST,PORT))
    data, addr = s.recvfrom(max_buf_len)
    print 'Ground Station: Received', repr(data), 'from', addr
    imgNum = 0
    while True:
        data = receiveImage()
        print 'Ground Station: got image',imgNum
        fname = 'img{:05d}.ppm'.format(imgNum)
        imgNum = imgNum + 1
        f = open(fname, "wb")
        header = 'P6 640 480 255 '
        f.write(header)
        for d in data:
            for elem in d:
                f.write('{}'.format(elem))
        f.close()
    s.close()
    return

if __name__ == "__main__":
    main()
