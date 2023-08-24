# Critical-Section-Problem
## Question 1:
Consider the following scenario. A town has a very popular restaurant. The restaurant can hold N diners. The number of people in the town who wish to eat at the restaurant, and are waiting outside
its doors, is much larger than N. The restaurant runs its service in the following manner. Whenever it is ready for service, it opens its front door and waits for diners to come in. Once N diners enter,
it closes its front door and proceeds to serve these diners. Once service finishes, the backdoor is opened and the diners are let out through the backdoor. Once all diners have exited, another batch
of N diners is admitted again through the front door. This process continues indefinitely. The restaurant does not mind if the same diner is part of multiple batches.

Write a C/C++ program to model the diners and the restaurant as threads in a multithreaded
program. The threads must be synchronized as follows.

* A diner cannot enter until the restaurant has opened its front door to let people in.
* The restaurant cannot start service until N diners have come in.
* The diners cannot exit until the back door is open.
* The restaurant cannot close the backdoor and prepare for the next batch until all the diners of the previous batch have left.

## Question 2:
Saraighat Bridge is the only bridge that connects the scenic north and south Guwahati and plays
a vital role as it is one of the busiest bridges in the region. The bridge can become deadlocked if
northbound and southbound people get on the bridge at the same time. (Let us assume the people
are stubborn and are unable to back up.) Write a C /C++ code to implement the scenario of bridge
using semaphores and/or mutex locks, that prevents deadlock.

➢ **Case 1** : Consider the scenario in which northbound people prevent southbound
people from using the bridge, or vice versa.<br>
➢ **Case 2** : Consider the scenario in which northbound people don't prevent
southbound people from using the bridge, or vice versa.

Represent northbound and southbound people as separate threads. Once a person is on the bridge,
the associated thread will sleep for a random period of time, representing travelling across the
bridge. Design your program so that you can create several threads representing the northbound
and southbound persons.
