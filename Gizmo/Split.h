#ifndef __SPLIT_H__
#define __SPLIT_H__



/////// THE KEY SPLITTER
//
// The key splitter can do any of the following:
//
// 1. Split a keyboard by a single note (options.splitNote).  Notes and poly aftertouch >= the split note will be played
//    out options.channelOut.  Notes < the split note will be played out options.splitChannel.  CC, NRPN, PC, Pitch Bend, and
//    channel aftertouch will be played out options.channelOut if options.splitControls == SPLIT_RIGHT, else they
//    will be played out the options.splitChannel.
//
// 2. Split a keyboard by two notes (options.splitNote and options.splitLayerNote) into THREE regions.  Notes and poly
//    aftertouch >= the split note will be played out options.channelOut.  Notes <= the split layer note will be played
//    out options.splitChannel.  Thus if options.splitNote <= options.splitLayerNote, this will create a region between
//    the two where notes are played out BOTH CHANNELS.  Similarly, if options.splitNote > options.splitLayerNote, this
//    will create a potential middle region where notes are played out NEITHER CHANNEL.  CC, NRPN, PC, and
//    channel aftertouch will be played out options.channelOut if options.splitControls == SPLIT_RIGHT, else they
//    will be played out the options.splitChannel.  Pitch bend will be played out both channels.  (This is basically layering). 
//
// 3. If options.splitControls == SPLIT_MIX, this does a fader/balance mechanism based on note velocity.  When a note is
//    played or released with a given velocity V, then this note is played as normal, with velocity V, out
//    options.channelOut: and ADDITIONALLY, the note is played with a velocity 127-V, out options.splitChannel.  
//    Poly aftertouch is sent to both channels with the same pressure amount.  CC, NRPN, PC, and channel aftertouch 
//    will be played out options.channelOut.  Pitch bend will be played out both channels.
//
// OPTIONS
//
// Permanent options special to the Key Splitter are:
//
// options.splitChannel               					What channel should be the "alternate" channel to play out?
//															This can be any value 1-16.
// options.splitNote									What should be the primary split note?  This can be any value 0..127
// options.splitLayerNote								What should be the secondary split note?  This can be any value 0..127
//															OR it can be NO_NOTE (128)
// options.splitControls								If SPLIT_RIGHT, CC etc. are sent to options.channelOut.
//															If SPLIT_LEFT, CC etc. are sent to options.splitChannel.
//															If SPLIT_MIX, the fader/balance mechanism occurs.
//
// Other permanent options affecting the Key Splitter include:
//
// options.channelIn
// options.channelOut
// options.transpose
// options.volume
//
// DISPLAY
// 
// If splitting on a single note, that note pitch is displayed in the left matrix.  Additionally, an LED (left matrix, (0,0))
// will light if SPLIT_LEFT, else (left matrix, 7, 0) will light if  SPLIT_RIGHT.
//
// If layering with two notes, the primary note pitch is displayed in the left matrix and the layer note pitch is displayed in
// the right matrix.  Additionally, an LED (left matrix, (0,0)) will light if SPLIT_LEFT, else (left matrix, 7, 0) will light if SPLIT_RIGHT.
//
// If doing the fader/balance mechanism, "FADE" is displayed.
//
// When choosing a split note (STATE_SPLIT_NOTE), you can select any note.
// When choosing a layer split note, you can select any note.  But if you already had selected one,
// you can only DESELECT a note ("----") [set the note to NO_NOTE].  Thereafter you can select a new note.
// When 
//
// INTERFACE
//
// Root
//      Split			STATE_SPLIT
//              Back Button: 	STATE_ROOT 
//				Select Button: 	STATE_SPLIT_NOTE
//				Middle Button:	STATE_SPLIT_LAYER_NOTE
//				Select Button Long Press:	cycle through SPLIT_RIGHT, SPLIT_LEFT, and SPLIT_MIX
//				Middle Button Long Press:	STATE_SPLIT_CHANNEL
//				Play a note: it's routed appropriately



#define SPLIT_CONTROLS_RIGHT	0		// this is the default, see options
#define SPLIT_CONTROLS_LEFT	1
#define SPLIT_MIX 2

void stateSplit();
void stateSplitNote();
void stateSplitLayerNote();

#endif // __SPLIT_H__