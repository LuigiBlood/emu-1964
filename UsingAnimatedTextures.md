# Explanation/Instructions #

The alternative(animated) texture functionality works by checking whether there is a text file called **texturenamehere**_infos.txt for each and every texture._

This _infos.txt defines parameters for the actual animated feature and tells the plugin how many alternate textures there are to load for it, and the speed at which to change to the alternate textures._


---


For instance, in the community texture repack for Zelda Ocarina of Time, we use the entrance into the fire temple, the name for this texture being

THE LEGEND OF ZELDA#3A14C086#0#2\_all.png

For this texture we have a .txt file with the filename:
THE LEGEND OF ZELDA#3A14C086#0#2\_infos.txt

Within this .txt file we specify several things, we are using the .txt file from the texture pack as an example.

```
#The amount of textures to be loaded in as alternatives
Count 5 
#Whether the textures should be shuffled and not displayed in order
Shuffle 0
#The time in milliseconds between each texture 
Period 60
#Whether you want this texture to be synchonised along with any others that have the option set to 1
Synchronized 0
```

Now since we have the count set as 5, the plugin is now expecting us to provide the plugin with those 5 textures.
Now one thing to know is that this system counts from 0 upwards, not from 1, so if you set it at 5, the end in your alt numbers is going to end at 4, like as we show below

```
THE LEGEND OF ZELDA#3A14C086#0#2_all.png <-- Original texture
THE LEGEND OF ZELDA#3A14C086#0#2_alt0_all.png <-- First alternative
THE LEGEND OF ZELDA#3A14C086#0#2_alt1_all.png <-- Second Alternative
THE LEGEND OF ZELDA#3A14C086#0#2_alt2_all.png <-- Third Alternative
THE LEGEND OF ZELDA#3A14C086#0#2_alt3_all.png <-- Fourth Alternative
THE LEGEND OF ZELDA#3A14C086#0#2_alt4_all.png <-- Fifth Alternative
```

In any case the alt0(And so forth alt1.. alt2.. alt3) must come before the texture type definition like as shown below.
```
 _ciByRGBA.png --> alt0_ciByRGBA.png
 _a.png        --> alt0_a.png
 _rgb.png      --> alt0_rgb.png
 _all.png      --> alt0_all.png
```

In all, if you find any faults with this tutorial please post comment with the problems you found and they should be fixed promptly