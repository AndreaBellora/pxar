pXar
====

pixel Xpert analysis &amp; readout

User guide: [main.pdf](https://github.com/psi46/pxar/blob/master/main.pdf)<br>
pxarCore documentation: [CMS-NOTE-2016-001](https://cds.cern.ch/record/2137512)

Please visit https://twiki.cern.ch/twiki/bin/view/CMS/Pxar for instructions on how to install and use the software.
Please visit http://psi46.github.io/annotated.html for the DOXYGEN documentation of the code. 

To allow pXar to find the DTB

  sudo nano /etc/udev/rules.d/00-usb-permissions.rules
 Add:
  SUBSYSTEM=="usb", MODE="0660", GROUP="dialout"
then
  sudo usermod -a -G dialout yourusername
restart and reconnect the DTB