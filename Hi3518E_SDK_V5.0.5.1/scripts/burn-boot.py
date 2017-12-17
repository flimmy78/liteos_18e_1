#!/usr/bin/python

import os
import serial
import array
import thread
import time
import sys


uboot_size = "0x40000" # the uboot size 3*64k=192k
uboot_name = "u_boot_hi3516c.bin" #the download-filename


ACK = 0xaa
time_15s = 15
is_close = 0
is_sendfile = 0
recv_data = 0

SEND_COMD = ["getinfo version;","getinfo bootmode;","getinfo spi;","sf probe 0;","sf erase 0x0 ", "sf write 0x81000000 0x0 "]

class bootdownload(object):
    '''
    Hisilicon boot downloader

    >>> downloader = bootdownload()
    >>> downloader.download(filename)

    '''

    # crctab calculated by Mark G. Mendel, Network Systems Corporation
    crctable = [
        0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
        0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
        0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
        0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
        0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
        0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
        0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
        0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
        0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
        0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
        0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
        0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
        0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
        0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
        0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
        0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
        0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
        0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
        0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
        0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
        0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
        0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
        0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
        0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
        0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
        0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
        0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
        0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
        0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
        0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
        0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
        0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0,
    ]

    configdata = {
	'hi3516cv200':[
	0x04,0xE0,0x2D,0xE5,0x24,0x00,0x9F,0xE5,0x24,0x10,0x9F,0xE5,0x00,0x10,0x80,0xE5,
        0x20,0x00,0x9F,0xE5,0x20,0x10,0x9F,0xE5,0x04,0x10,0x80,0xE4,0x00,0xE0,0x80,0xE5,
        0x04,0xF0,0x9D,0xE4,0xEF,0xBE,0xAD,0xDE,0xEF,0xBE,0xAD,0xDE,0xEF,0xBE,0xAD,0xDE,
        0x3C,0x01,0x05,0x20,0x4E,0x57,0x4F,0x44,0x40,0x01,0x05,0x20,0x75,0x6A,0x69,0x7A
        ]
    }

    startframe = {
	'hi3516cv200':[0xFE,0x00,0xFF,0x01,0x00,0x00,0x00,0x04,0x00,0x00,0x02,0x01]
    }

    headframe = {
	'hi3516cv200':[0xFE,0x00,0xFF,0x01,0x00,0x00,0x00,0x04,0x00,0x00,0x02,0x01]
    }

    configaddress = {
	'hi3516cv200':0x04013000
    }

    bootheadaddress = {
        'hi3516cv200':0x04010500
    }

    bootdownloadaddress = {
        'hi3516cv200':0x81000000
    }

    BOOT_HEAD_LEN = 0x3b00
    MAX_DATA_LEN  = 0x400

    def __init__(self,chiptype,serialport):
        self.s = serial.Serial(port=serialport, baudrate=115200)
        self.chip = chiptype

    def __del__(self):
        self.s.close()

    def calc_crc(self, data, crc=0):
        for char in data:
            crc = ((crc << 8) | ord(char)) ^ self.crctable[(crc >> 8) & 0xff]
        for i in range(0,2):
            crc = ((crc << 8) | 0) ^ self.crctable[(crc >> 8) & 0xff]
        return crc & 0xffff

    def getsize(self, filename):
        st = os.stat(filename)
        return st.st_size

    def sendframe(self, data, loop):
        for i in range(1, loop):
            self.s.flushOutput()
            self.s.write(data)
            self.s.flushInput()
            ack = self.s.read()
            if len(ack) == 1:
                if ack == chr(0xaa):
                    return None
        print ('failed')

    def sendstartframe(self):
        self.s.setTimeout(0.01)
        data = array.array('B', self.startframe[self.chip]).tostring()
        crc = self.calc_crc(data)
        data += chr((crc >> 8)&0xff)
        data += chr(crc&0xff)
        self.sendframe(data,10000)

    def sendheadframe(self,length,address):
        self.s.setTimeout(0.03)
        self.headframe[self.chip][4] = (length>>24)&0xff
        self.headframe[self.chip][5] = (length>>16)&0xff
        self.headframe[self.chip][6] = (length>>8)&0xff
        self.headframe[self.chip][7] = (length)&0xff
        self.headframe[self.chip][8] = (address>>24)&0xff
        self.headframe[self.chip][9] = (address>>16)&0xff
        self.headframe[self.chip][10] = (address>>8)&0xff
        self.headframe[self.chip][11] = (address)&0xff

        data = array.array('B', self.headframe[self.chip]).tostring()
        crc = self.calc_crc(data)

        data += chr((crc >> 8)&0xff)
        data += chr(crc&0xff)

        self.sendframe(data,16)


    def senddataframe(self,seq,data):
        self.s.setTimeout(0.15)
        head = chr(0xDA)
        head += chr(seq&0xFF)
        head += chr((~seq)&0xFF)

        data = head + data

        crc = self.calc_crc(data)
        data += chr((crc >> 8)&0xff)
        data += chr(crc&0xff)

        self.sendframe(data,32)

    def sendtailframe(self,seq):
        data = chr(0xED)
        data += chr(seq&0xFF)
        data += chr((~seq)&0xFF)
        crc = self.calc_crc(data)
        data += chr((crc >> 8)&0xff)
        data += chr(crc&0xff)

        self.sendframe(data,16)

    def senddata(self,data,address):
        global is_sendfile
        length=len(data)
        length0 = len(data)
        self.sendheadframe(length,address)
        seq=1
        num0 = 0
        while length > self.MAX_DATA_LEN:
            self.senddataframe(seq,data[(seq-1)*self.MAX_DATA_LEN:seq*self.MAX_DATA_LEN])
            seq = seq+1
            length = length-self.MAX_DATA_LEN
            if is_sendfile == 1:
                num = (seq*self.MAX_DATA_LEN*100)/length0
                if num == 10 or num == 20 or num == 30 or num == 40 \
                   or num == 50 or num == 60 or num == 70 or num == 80\
                   or num == 90:
                    if num0 != num:
                        num0 = num
                        print "%d%%"%(num)
                else:
                    sys.stdout.write("#")
        
        self.senddataframe(seq,data[(seq-1)*self.MAX_DATA_LEN:])
        if is_sendfile == 1:
            print "100%"
        self.sendtailframe(seq+1)

    def sendcmdhead(self,length):
        data = chr(0xAB)
        data += chr((length >> 8)&0xff)
        data += chr(length & 0xff)
        crc = self.calc_crc(data)
        data += chr((crc>>8) & 0xff)
        data += chr(crc & 0xff)
        self.sendframe(data, 16)

    def sendcmd(self, cmd):
        data = chr(0xCD)
        data += cmd
        crc = self.calc_crc(data)
        data += chr((crc>>8) & 0xff)
        data += chr(crc & 0xff)
        self.sendframe(data, 32)
        
        
    def check_is_close_port(self, threadName):
        global time_15s
        global is_close
        global recv_data
        
        while time_15s > 0 and recv_data == 0:
            time.sleep(1)
            time_15s -= 1
            #print "time_15s[%d]"%(time_15s)
        else:
            if recv_data == 0:
                is_close = 1
                print "over time! recev anything in 15s \n"
                self.s.close()
                sys.exit(1)
		
    def download(self, filename):
        global is_close
        global is_sendfile
        count = 0;
        thread.start_new_thread( self.check_is_close_port, ("check-time", ) )
        
        sys.stdout.write("SerialPort has been connencted, Please power off, then power on the device.\nIf it doesn't work, please try to repower on.\n")
        while is_close == 0:
            for i in range(0,4):
                if is_close == 0:
                    value = self.s.read(1)
                    #print ('recv data %s:%d')%(value, count)
                    if value == " ":
                        count = count+1
                    else:
                        count = 0
            if count >= 5:
                global recv_data
                recv_data = 1;
                break
        else:
            sys.exit(1)
		
        self.s.write(chr(ACK))#send ack
        
        #print ('sending start frame...')
        self.sendstartframe()
        #print ('done \n')
        #print ('sending config info...chip[%s]')%(self.chip)
        data = array.array('B',self.configdata[self.chip]).tostring()
        self.senddata(data,self.configaddress[self.chip])
        #print ('done\n')

        f=open(filename,"rb")
        data = f.read()
        f.close()

        if len(data) < self.BOOT_HEAD_LEN:
            print ('boot file length error')
            return

        #print ('sending boot head...')
        self.senddata(data[:self.BOOT_HEAD_LEN],self.bootheadaddress[self.chip])
        #print ('done\n')

        #print ('sending boot to flash...')
        is_sendfile = 1
        self.senddata(data,self.bootdownloadaddress[self.chip])
        print ('Boot download completed!\n')

    def Send_commod(self):
        global time_15s        
        global recv_data
        global uboot_size
        global SEND_COMD
        time_15s = 10
        recv_data = 0
        thread_check = thread.start_new_thread( self.check_is_close_port, ("check-time", ) )
        step = 0
        while True:
            data = self.s.readline()
            print data
            if step >= 1:
                if data.find("version:") >= 0:
                    recv_data = 1
                    version = data[(data.find(":")+2):(len(data)-2)]
                elif data.find("getinfo") < 0 and data.find("spi") >= 0:
                    recv_data = 1
                    bootmode = "spi"
                elif data.find("Block:") >= 0:
                    recv_data = 1
                    block = data[data.find(":"):data.find(" ")]
                    chip_size = data[data.find("Chip:")+len("Chip:"):len(data)-2]
                elif data.find("ID:") >= 0:
                    recv_data = 1
                    Id = data[data.find("ID:")+len("ID:"):len(data)-2]
                elif data.find("Name:") >= 0:
                    recv_data = 1
                    name = data[data.find("Name:")+len("Name:"):len(data)-2]
                elif data.find("(OK)"):
                    if step == 4:
                        recv_data = 1
                        SEND_COMD[step] += uboot_size
                        SEND_COMD[step] += ';'
                        print "Send Command: %s"%(SEND_COMD[step])
                        self.sendcmdhead(len(SEND_COMD[step]))
                        self.sendcmd(SEND_COMD[step])
                        time.sleep(1)
                        step += 1
                    elif step == 5:
                        time.sleep(1)
                        recv_data = 1
                        SEND_COMD[step] += uboot_size
                        SEND_COMD[step] += ';'
                        print "Send Command: %s"%(SEND_COMD[step])
                        self.sendcmdhead(len(SEND_COMD[step]))
                        self.sendcmd(SEND_COMD[step])
                        step += 1
                    elif step >= 6:
                        recv_data = 1
                        self.s.close()
                        sys.exit(1)
                        return
                    else:
                        time_15s = 10
                        recv_data = 0
                        print "Send Command: %s"%(SEND_COMD[step])
                        self.sendcmdhead(len(SEND_COMD[step]))
                        self.sendcmd(SEND_COMD[step])
                        step += 1
                    
                    data = ""
                    
                else:
                    pass

            if step == 0 and data.find("start download process.") >= 0 :#just boot is up
                print "Send Command: %s"%(SEND_COMD[step])
                self.sendcmdhead(len(SEND_COMD[step]))
                self.sendcmd(SEND_COMD[step])
                step = 1
            
def burnboot(chiptype, serialport=0, filename='fastboot-burn.bin'):
    downloader = bootdownload(chiptype, serialport)
    downloader.download(filename)
    downloader.Send_commod()

def startterm(serialport=0):
    try:
        miniterm = Miniterm(
            serialport,
            115200,
            'N',
            rtscts=False,
            xonxoff=False,
            echo=False,
            convert_outgoing=2,
            repr_mode=0,
            )
    except serial.SerialException, e:
        sys.stderr.write("could not open port %r: %s\n" % (port, e))
        sys.exit(1)
    miniterm.start()
    miniterm.join(True)
    miniterm.join()

def main_():
    serialport = raw_input("Please enter the serial(example: COM1\COM2\COM3...): ")
    #print serialport
    burnboot('hi3516cv200', serialport, uboot_name)

main_()
