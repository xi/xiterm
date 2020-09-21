I spent some time switching between gnome-terminal, xfce4-terminal, and
lxterminal trying to find the perfect configuration. Eventually I decided to
cut out the middleman and use libvte directly. I published it mostly as a
reference for others who want to do the same.

# Installation

	sudo apt install libgtk-3-dev libvte-2.91-dev
	make
	sudo make install
	sudo update-alternatives --install /usr/bin/x-terminal-emulator x-terminal-emulator /usr/bin/xiterm 100
	echo ". /etc/profile.d/vte-2.91.sh" >> ~/.bashrc

# Features

-	configure by changing the code
-	custom color scheme
-	change font scale with Ctrl-+|-|0
-	open http URLs with right click
-	new tabs open after current one
-	switching tabs does not wrap around
