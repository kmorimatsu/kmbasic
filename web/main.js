/**********************************
* KM-BASIC web written by Katsumi *
*     This script is released     *
*       under the LGPL v2.1.      *
**********************************/

display.init();
interrupt.init();
if (get.debug) dom.getElement("code").style.display="block";
if (get.help) dom.getElement("side").style.display="block";
dom.getElement("RUNbutton").disabled=0;
