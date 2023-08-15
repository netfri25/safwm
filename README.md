# safwm
Simple AF window manager

## But why another window manager?
I'm a man of simplicity.
I used many window managers for a really long time (about a week) and I've came to the realization of what is the best window manager that can be created (imo ofc).\
It's important to know that I can't please everyone, so a "must be" thing is source code readability and ease of modification - meaning that everyone who wants to can modify/add/remove from the source code without any problem.

## TLDR
simplicity.

### There are some window managers that almost met my expectancies:
* [Ragnar](https://github.com/cococry/Ragnar) - The first ever window manager that I've used. truly an amazing piece of software, but the source was hard to modify, although it was easy to understand.
* [sowm](https://github.com/dylanaraps/sowm) - Really amazing window manager. most of my source code is inspired (stolen) from it. the only problem I had is variable naming (hard to understand the difference between `client.f` and `client.fs` - which one is focus? which one is fullscreen? both words contain `f` and `s`...) and weird control flow (it took me about an hour to understand `for win if`, until I figured out that `win` is actually a macro).\
  sowm also created empty windows for background applications and left them there, and it also windows became invisible when trying to do a screenshot and I couldn't figure out why (this is the thing that made me decide on starting to write my own window manager).
* [dwm](https://dwm.suckless.org/) - the first minimal window manager I've encountered (but not the first one I've settled on using), but the source code was just... weird? and I was too lazy to actually understand how it works.
* [xmonad](https://xmonad.org/) - I'm in love Haskell for about a year and a half already, but XMonad made the installation proccess and configuration a lot more complicated than it should be :(

## Installation
```shell
$ git clone https://github.com/netfri25/safwm
$ cd safwm
$ sudo make install
```
if you are using a session manager, then also run:
```shell
$ sudo cp safwm.desktop /usr/share/xsessions
```
if you are using startx (why) then just add this to the end of your `xinit` script:
```conf
exec safwm
```

## Keybindings
###### (more will be added in the future)
**General window manager keybindings**
| Key Combo               | action |
| ----------------------- | ------- |
| `Super` + `Shift` + `q` | quit safwm |

**Window related keybindings**
| Key Combo       | action             |
| --------------- | ------------------- |
| `Mouse`         | focus under cursor  |
| `Super` + `LMB` | move window         |
| `Super` + `RMB` | resize window       |
| `Super` + `c`   | center window       |
| `Super` + `m`   | maximize window     |
| `Super` + `f`   | fullscreen window   |
| `Super` + `q`   | quit window         |

**Applications keybindings**
| Key Combo          | action               | command                          |
| ------------------ | -------------------- | -------------------------------- |
| `Super` + `s`      | application launcher | `rofi -show drun -theme gruvbox` |
| `Super` + `Return` | terminal             | `alacritty`                      |