# M031BSP_UART_SimpleCmd_II
 M031BSP_UART_SimpleCmd_II


update @ 2022/11/08

1. initial UART to receive RX command 

UART receive command and parsing function refer below sample code , 

http://forum.nuvoton.com/viewtopic.php?f=19&t=10995&sid=248d23521df8ad0263f4e301d2641a63

2. by using terminal tool , send UART command to MCU to parsing command

head + function code + data0 + data1 + data2 + data3 + checksum(CRC8) + tail

when UART command receive and parsing CORRECT

![image](https://github.com/released/M031BSP_UART_SimpleCmd_II/blob/main/correct.jpg)	


when UART command receive and parsing HEAD incorrect

![image](https://github.com/released/M031BSP_UART_SimpleCmd_II/blob/main/error_head.jpg)	


when UART command receive and parsing funtion code incorrect

![image](https://github.com/released/M031BSP_UART_SimpleCmd_II/blob/main/error_code.jpg)	


when UART command receive and parsing checksum incorrect

![image](https://github.com/released/M031BSP_UART_SimpleCmd_II/blob/main/error_cs.jpg)	


when UART command receive and parsing TAIL incorrect

![image](https://github.com/released/M031BSP_UART_SimpleCmd_II/blob/main/error_tail.jpg)


