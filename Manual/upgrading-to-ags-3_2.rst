.. _UpgradeTo32:

Upgrading to AGS 3.2
####################

AGS 3.2 contains some major changes, the main one being a completely new way of
scripting the game audio.

Why a new audio system?
=======================

In previous versions of AGS, sound and music was a pain to use. Although it was
very simple to script using commands like PlayMusic(5) and PlaySound(10), the
fact that it was so basic became a limitation.

What is music 5? Which sound effect is Sound 10? How are you supposed to remember?
It was all a bit chaotic and old-fashioned. Furthermore, controlling the volume
involved several different commands, making it something of a black art.

So what's changed?
==================

The old commands like PlayMusic and PlaySound have been obsoleted now, and replaced
with a new object-based audio system. This means that audio files are now represented
by script instances in the game.

For example, in AGS 3.1.2, you might have done this::

   PlaySound(10); // this is an explosion


Now, with AGS 3.2 you would do it like this instead::

   aExplosion.Play();


So how do I name my audio files?
================================

There is a new "Audio" item in the project tree, which you now use to manage your audio.
By default, when you import your game from a previous version of AGS, it will create
audio clips with names like "aMusic5" and "aSound30", corresponding to their old names.

If you want the old-style commands like PlayMusic and PlaySound to continue working,
then **you must not rename these audio clips**. AGS maintains a backwards compatibility
layer by mapping the PlayMusic(X) command to play an audio clip called "aMusicX", and
the PlaySound(X) command to play a clip called "aSoundX".

Otherwise, if you want to convert your scripts to the new audio system, you can simply
right-click and rename these audio clips as you see fit.

There is now an AudioCache folder, do I still need the Sound and Music folders?
===============================================================================

When you import an audio file into AGS, it makes a copy of it in the AudioCache folder,
but it also remembers where the file came from originally. If the original file changes,
AGS will automatically copy the updated file into the AudioCache folder.

When you upgrade an old game, the original file location for where AGS thinks your audio
files came from is set to the "Sound" and "Music" folders. Therefore, keeping these folders
is advisable initially as it allows you to continue to update the existing files in the same way
you always have done, and AGS will automatically pick up the changes.

But going forward, as you import new audio files, there's no need to have them in the Sound
or Music folders; import them from wherever you like.

What about controlling the volume?
==================================

Glad you asked! Instead of all those old commands like SetMusicVolume, SetDigitalMasterVolume,
etc, there is now simply one overall system volume (:ref:`System.Volume <System.Volume>`), and
then each sound that is playing has its own volume control as well. This is controlled by the
:ref:`Volume property <AudioChannel.Volume>` on the audio channel (see the :ref:`Audio page <MusAndSound>`
for details on this).

Finally, you can update the volume of one particular type of audio (eg. sound, music) by using
the :ref:`Game.SetAudioTypeVolume <Game.SetAudioTypeVolume>` command.

Wait, what's an audio channel?
==============================

AGS uses two new concepts -- Audio Clips (which represent a particular sound file), and Audio Channels
(which represent a currently playing piece of audio). The reason for this distinction is that you might
have the same sound effect simultaneously playing two or more times, and having Audio Channels allows you to
control each one individually. The :ref:`Audio page <MusAndSound>` describes this further.

PlayAmbientSound is obsolete! How do I do ambient sounds?
=========================================================

Ambient Sounds were a bit of an oddity in AGS, caused by the fact that you couldn't tell a PlaySound
command to loop the sound. With the new audio system, the :ref:`Play command <AudioClip.Play>` has
an optional Repeat parameter, allowing you to specify whether it loops or not.

The X/Y directional aspect of PlayAmbientSound is supported by the :ref:`SetRoomLocation <AudioChannel.SetRoomLocation>`
command on the audio channel.

Is there any new cool stuff that I can do?
==========================================

You can now adjust the left-right panning of audio, using the :ref:`AudioChannel.Panning <AudioChannel.Panning>`
property. You also have finer control over syncing up different pieces of audio, through the ability
to get and seek offsets more accurately.

Has voice speech changed?
=========================

No, speech is still handled the same way as in previous versions of AGS. You still need
the Speech folder within your game folder, and name the files within it using the same
naming convention as you always have done.

Where should I look for more info?
==================================

See the :ref:`Audio help <MusAndSound>` for more information.
