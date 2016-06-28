Well here it is, as promised: the open source release of Barony. Keep in mind you still need a purchased copy of Barony to play this. I'd recommend that you thumb through all of the included text files to get a feeling of other things you'll need to build the game and check out the included licenses as well.

Many thanks go to Ciprian Ilies for his original contributions to the game code, as well as for the build systems, config files, and support libraries that he developed for the project over the years. In the future, he plans to head up development on some new stuff for Barony, so keep an eye out for that.

This project was a first for both of us in many ways and it shows. Since all of the original code was written in C and hastily converted to C++ in the past few months, experienced C++ programmers may be horrified at some of the kludge we had to write to get some of the more basic systems working properly. There's not a lot of module organization either since I didn't understand how to properly write projects that scale when I started the code three years ago. Prepare to deal with lots of global variables that get used all over the project indiscriminately.

Despite the project's shortcomings, I'm reasonably proud of how the end product turned out. Writing good games is about more than just writing good code, though I guarantee we'll be taking all of the lessons learned from Barony into our next project.

I'm not sure how many people will be interested in working on this, and it may take a while for anything substantial to get going here, but I'd be pleased to see some coordinated efforts take place on this code sometime in the coming years.

Some project ideas:

 * Add an extra hard mode to the game.
 * Add a dungeon with infinite levels.
 * Create a dedicated server.
 * Multithread the packet handler.
 * Multithread the entity logic.
 * Add script support for entities and items.
 * Add persistent levels and servers.
 * Add fully 3D physics and world geometry.
 * Renovate the OpenGL code to a modern standard.

Have fun,

Sheridan
June 27th 2016
