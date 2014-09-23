.. _AnonymousUsageInfo:

Anonymous usage information
###########################

AGS contains an option to send anonymous usage information to the AGS website.
But what is it all about and why would you want to do so?

What information does it send?
==============================

The following is the information that AGS currently sends to the server.


* AGS version
* Windows version (eg. XP, Vista)
* .NET Framework version
* Screen resolution


That's it. Nothing else is sent, no personal details or scripts from your games
or anything like that.

Why should I enable this?
=========================

By having a clearer picture of what types of system people are running, it
allows us to make better decisions about future versions of AGS. Here are some
examples of where it will be useful:


* A serious bug is found that affects an older version of AGS. By knowing how
  many people are still using that version, we can decide whether to fix it or not.
* We want to add a feature to the AGS Editor that would only work on newer versions of Windows. By
  knowing how many people are running these, it gives us a clear idea of how many
  people would stand to benefit and therefore whether it's worth doing.
* The AGS Editor GUIs can be designed to work with the screen resolutions
  that most people are using. By knowing what the minimum resolution is that people
  still use, that guides how the editor is designed.


If you object to this very basic information being submitted to AGS, you can
turn it off in the Preferences window. However, by doing so you may lose out in
the long run if AGS features are developed that will not work on your system.

When does it send this information?
===================================

The editor contacts the AGS website once a month and sends it these details.

Will games that I make contact the AGS website?
===============================================

No, only the editor does this.
