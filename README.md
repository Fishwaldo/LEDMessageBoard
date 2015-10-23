# LEDMessageBoard
A Linux Driver for a LED Message Board. 

This is a little program I wrote to update the message displayed on a Led Message Board.

The Message Board is available here: http://www.dx.com/p/programmable-scrolling-led-name-message-advertising-tag-badge-red-light-107826

There are two ways to update messages on the board:

1) via a telnet connection to port 8000 (default) with a userid of Admin and password of "password" (can be set in /etc/LMBd.cfg)
2) writting to a file located in /var/spool/LMBd/message-<1-6> where 1-6 represents the message slot.

There is also a sample python script that pulls some info from a zabbix server and displays it on the board.

Any questions? just raise a new issue. 

