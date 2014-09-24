/* March 19, 2004 MathHTML (c) Peter Jipsen http://www.chapman.edu/~jipsen
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.
This program is distributed in the hope that it will be useful, but 
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License 
(at http://www.gnu.org/copyleft/gpl.html) for more details.*/

function convertMath(node) {// for Gecko
  if (node.nodeType==1) {
    var newnode = 
      document.createElementNS("http://www.w3.org/1998/Math/MathML",
        node.nodeName.toLowerCase());
    for(var i=0; i < node.attributes.length; i++) {
	    if (node.attributes[i].nodeName == 'displaystyle') {
	      newnode.setAttribute(node.attributes[i].nodeName,node.attributes[i].nodeValue);
	    }
    }
    for (var i=0; i<node.childNodes.length; i++) {
      var st = node.childNodes[i].nodeValue;
      if (st==null || st.slice(0,1)!=" " && st.slice(0,1)!="\n") 
        newnode.appendChild(convertMath(node.childNodes[i]));
    }
    return newnode;
  }
  else return node;
}
function convert() {
	
	if (document.createElementNS) {
		var mmlnode = document.getElementsByTagName("span");
		
		for (var i=0; i<mmlnode.length; i++) {
			var tmp_node = mmlnode[i];
			if (tmp_node.className == 'asciimath') {
				tmp_node.replaceChild(convertMath(tmp_node.firstChild),tmp_node.firstChild);
				/*
				for (var j=0;j<tmp_node.childNodes.length;j++) {
					if (tmp_node.childNodes[j].nodeType != 3) {
						
					}
				}
				*/
			}
		}
	} else {
		var st,node,newnode;
		var mmlnode = document.getElementsByTagName("math");
		
	  	for (var i=0; i<mmlnode.length; i++) {
			var str = "";
			node = mmlnode[i];
			while (node.nodeName!="/MATH" && node.nextSibling) {
				st = node.nodeName.toLowerCase();
				if (st=="#text") { 
					str += node.nodeValue;
				} else {
					str += (st.slice(0,1)=="/" ? "</m:"+st.slice(1) : "<m:"+st);
					if (st.slice(0,1)!="/") {
						for(var j=0; j < node.attributes.length; j++) {
							if (node.attributes[j].nodeValue!="italic" &&
							 node.attributes[j].nodeValue!="" &&
							 node.attributes[j].nodeValue!="inherit" &&
							 node.attributes[j].nodeValue!=undefined) {
							 str += " "+node.attributes[j].nodeName+"="+
							     "\""+node.attributes[j].nodeValue+"\"";
							}
						}
					}
					str += ">";
				}
				node = node.nextSibling;
				node.parentNode.removeChild(node.previousSibling);
			}
			
			if (str != '') {
				str += "</m:math>";
				newnode = document.createElement("span");
				node.parentNode.replaceChild(newnode,node);
				newnode.innerHTML = str;
			}
		}
	}
}