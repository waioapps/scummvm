/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */

#include "sci/sfx/iterator.h"	// for SongIteratorStatus
#include "sci/sfx/music.h"
#include "sci/sfx/soundcmd.h"

namespace Sci {

#define USE_OLD_MUSIC_FUNCTIONS

#define SCI1_SOUND_FLAG_MAY_PAUSE        1 /* Only here for completeness; The interpreter doesn't touch this bit */
#define SCI1_SOUND_FLAG_SCRIPTED_PRI     2 /* but does touch this */

#define FROBNICATE_HANDLE(reg) ((reg).segment << 16 | (reg).offset)
#define DEFROBNICATE_HANDLE(handle) (make_reg((handle >> 16) & 0xffff, handle & 0xffff))

/* Sound status */
enum {
	_K_SOUND_STATUS_STOPPED = 0,
	_K_SOUND_STATUS_INITIALIZED = 1,
	_K_SOUND_STATUS_PAUSED = 2,
	_K_SOUND_STATUS_PLAYING = 3
};

static void script_set_priority(ResourceManager *resMan, SegManager *segMan, SfxState *state, reg_t obj, int priority) {
	int song_nr = GET_SEL32V(segMan, obj, number);
	Resource *song = resMan->findResource(ResourceId(kResourceTypeSound, song_nr), 0);
	int flags = GET_SEL32V(segMan, obj, flags);

	if (priority == -1) {
		if (song->data[0] == 0xf0)
			priority = song->data[1];
		else
			warning("Attempt to unset song priority when there is no built-in value");

		flags &= ~SCI1_SOUND_FLAG_SCRIPTED_PRI;
	} else flags |= SCI1_SOUND_FLAG_SCRIPTED_PRI;

	state->sfx_song_renice(FROBNICATE_HANDLE(obj), priority);
	PUT_SEL32V(segMan, obj, flags, flags);
}

SongIterator *build_iterator(ResourceManager *resMan, int song_nr, SongIteratorType type, songit_id_t id) {
	Resource *song = resMan->findResource(ResourceId(kResourceTypeSound, song_nr), 0);

	if (!song)
		return NULL;

	return songit_new(song->data, song->size, type, id);
}

void process_sound_events(EngineState *s) { /* Get all sound events, apply their changes to the heap */
	int result;
	SongHandle handle;
	int cue;
	SegManager *segMan = s->_segMan;

	if (getSciVersion() > SCI_VERSION_01)
		return;
	// SCI1 and later explicitly poll for everything

	while ((result = s->_sound.sfx_poll(&handle, &cue))) {
		reg_t obj = DEFROBNICATE_HANDLE(handle);
		if (!s->_segMan->isObject(obj)) {
			warning("Non-object %04x:%04x received sound signal (%d/%d)", PRINT_REG(obj), result, cue);
			return;
		}

		switch (result) {

		case SI_LOOP:
			debugC(2, kDebugLevelSound, "[process-sound] Song %04x:%04x looped (to %d)\n",
			          PRINT_REG(obj), cue);
			/*			PUT_SEL32V(segMan, obj, loops, GET_SEL32V(segMan, obj, loop) - 1);*/
			PUT_SEL32V(segMan, obj, signal, SIGNAL_OFFSET);
			break;

		case SI_RELATIVE_CUE:
			debugC(2, kDebugLevelSound, "[process-sound] Song %04x:%04x received relative cue %d\n",
			          PRINT_REG(obj), cue);
			PUT_SEL32V(segMan, obj, signal, cue + 0x7f);
			break;

		case SI_ABSOLUTE_CUE:
			debugC(2, kDebugLevelSound, "[process-sound] Song %04x:%04x received absolute cue %d\n",
			          PRINT_REG(obj), cue);
			PUT_SEL32V(segMan, obj, signal, cue);
			break;

		case SI_FINISHED:
			debugC(2, kDebugLevelSound, "[process-sound] Song %04x:%04x finished\n",
			          PRINT_REG(obj));
			PUT_SEL32V(segMan, obj, signal, SIGNAL_OFFSET);
			PUT_SEL32V(segMan, obj, state, _K_SOUND_STATUS_STOPPED);
			break;

		default:
			warning("Unexpected result from sfx_poll: %d", result);
			break;
		}
	}
}

#define SOUNDCOMMAND(x) _soundCommands.push_back(new SciSoundCommand(#x, &SoundCommandParser::x))

SoundCommandParser::SoundCommandParser(ResourceManager *resMan, SegManager *segMan, SfxState *state, AudioPlayer *audio, SciVersion doSoundVersion) : 
	_resMan(resMan), _segMan(segMan), _state(state), _audio(audio), _doSoundVersion(doSoundVersion) {

	_hasNodePtr = (_doSoundVersion != SCI_VERSION_0_EARLY);

#ifndef USE_OLD_MUSIC_FUNCTIONS
	_music = new SciMusic();
	_music->init();
#endif

	switch (doSoundVersion) {
	case SCI_VERSION_0_EARLY:
		SOUNDCOMMAND(cmdInitHandle);
		SOUNDCOMMAND(cmdPlayHandle);
		SOUNDCOMMAND(cmdDummy);
		SOUNDCOMMAND(cmdDisposeHandle);
		SOUNDCOMMAND(cmdMuteSound);
		SOUNDCOMMAND(cmdStopHandle);
		SOUNDCOMMAND(cmdSuspendHandle);
		SOUNDCOMMAND(cmdResumeHandle);
		SOUNDCOMMAND(cmdVolume);
		SOUNDCOMMAND(cmdHandlePriority);
		SOUNDCOMMAND(cmdFadeHandle);
		SOUNDCOMMAND(cmdGetPolyphony);
		SOUNDCOMMAND(cmdGetPlayNext);
		break;
	case SCI_VERSION_1_EARLY:
		SOUNDCOMMAND(cmdVolume);
		SOUNDCOMMAND(cmdMuteSound);
		SOUNDCOMMAND(cmdDummy);
		SOUNDCOMMAND(cmdGetPolyphony);
		SOUNDCOMMAND(cmdUpdateHandle);
		SOUNDCOMMAND(cmdInitHandle);
		SOUNDCOMMAND(cmdDisposeHandle);
		SOUNDCOMMAND(cmdPlayHandle);
		SOUNDCOMMAND(cmdStopHandle);
		SOUNDCOMMAND(cmdSuspendHandle);
		SOUNDCOMMAND(cmdFadeHandle);
		SOUNDCOMMAND(cmdUpdateCues);
		SOUNDCOMMAND(cmdSendMidi);
		SOUNDCOMMAND(cmdReverb);
		SOUNDCOMMAND(cmdHoldHandle);
		break;
	case SCI_VERSION_1_LATE:
		SOUNDCOMMAND(cmdVolume);
		SOUNDCOMMAND(cmdMuteSound);
		SOUNDCOMMAND(cmdDummy);
		SOUNDCOMMAND(cmdGetPolyphony);
		SOUNDCOMMAND(cmdGetAudioCapability);
		SOUNDCOMMAND(cmdSuspendSound);
		SOUNDCOMMAND(cmdInitHandle);
		SOUNDCOMMAND(cmdDisposeHandle);
		SOUNDCOMMAND(cmdPlayHandle);
		SOUNDCOMMAND(cmdStopHandle);
		SOUNDCOMMAND(cmdSuspendHandle);
		SOUNDCOMMAND(cmdFadeHandle);
		SOUNDCOMMAND(cmdHoldHandle);
		SOUNDCOMMAND(cmdDummy);
		SOUNDCOMMAND(cmdSetHandleVolume);
		SOUNDCOMMAND(cmdSetHandlePriority);
		SOUNDCOMMAND(cmdSetHandleLoop);
		SOUNDCOMMAND(cmdUpdateCues);
		SOUNDCOMMAND(cmdSendMidi);
		SOUNDCOMMAND(cmdReverb);
		SOUNDCOMMAND(cmdUpdateVolumePriority);
		break;
	default:
		warning("Sound command parser: unknown DoSound type %d", doSoundVersion);
		break;
	}
}

SoundCommandParser::~SoundCommandParser() {
}

reg_t SoundCommandParser::parseCommand(int argc, reg_t *argv, reg_t acc) {
	uint16 command = argv[0].toUint16();
	reg_t obj = (argc > 1) ? argv[1] : NULL_REG;
	SongHandle handle = FROBNICATE_HANDLE(obj);
	int value = (argc > 2) ? argv[2].toSint16() : 0;
	_acc = acc;

	if (argc > 5) {	// for cmdSendMidi
		_midiCmd = argv[3].toUint16() == 0xff ?
					  0xe0 : /* Pitch wheel */
					  0xb0; /* argv[3].toUint16() is actually a controller number */
		_controller = argv[3].toUint16();
		_param = argv[4].toUint16();
	}

	if (command < _soundCommands.size()) {
		debugC(2, kDebugLevelSound, "%s", _soundCommands[command]->desc);
		(this->*(_soundCommands[command]->sndCmd))(obj, handle, value);
	} else {
		warning("Invalid sound command requested (%d), valid range is 0-%d", command, _soundCommands.size() - 1);
	}

	return _acc;
}

void SoundCommandParser::cmdInitHandle(reg_t obj, SongHandle handle, int value) {
	if (!obj.segment)
		return;

#ifdef USE_OLD_MUSIC_FUNCTIONS
	SongIteratorType type = (_doSoundVersion == SCI_VERSION_0_EARLY) ? SCI_SONG_ITERATOR_TYPE_SCI0 : SCI_SONG_ITERATOR_TYPE_SCI1;
	int number = GET_SEL32V(_segMan, obj, number);

	if (_hasNodePtr) {
		if (GET_SEL32V(_segMan, obj, nodePtr)) {
			_state->sfx_song_set_status(handle, SOUND_STATUS_STOPPED);
			_state->sfx_remove_song(handle);
		}
	}

	// Some games try to init non-existing sounds (e.g. KQ6)
	if (_doSoundVersion == SCI_VERSION_1_LATE) {
		if (!obj.segment || !_resMan->testResource(ResourceId(kResourceTypeSound, value)))
			return;
	}

	_state->sfx_add_song(build_iterator(_resMan, number, type, handle), 0, handle, number);

	if (!_hasNodePtr)
		PUT_SEL32V(_segMan, obj, state, _K_SOUND_STATUS_INITIALIZED);
	else
		PUT_SEL32(_segMan, obj, nodePtr, obj);

	PUT_SEL32(_segMan, obj, handle, obj);
#else

	uint16 resnum = GET_SEL32V(_segMan, obj, number);
	Resource *res = resnum ? _resMan->findResource(ResourceId(kResourceTypeSound, resnum), true) : NULL;

	if (!GET_SEL32V(_segMan, obj, nodePtr)) {
		PUT_SEL32(_segMan, obj, nodePtr, obj);
		_soundList.push_back(obj.toUint16());
	}

	// TODO
	/*
	sciSound *pSnd = (sciSound *)heap2Ptr(hptr);
	pSnd->resnum = resnum;
	pSnd->loop = (GET_SEL32V(_segMan, obj, loop) == 0xFFFF ? 1 : 0);
	pSnd->prio = GET_SEL32V(_segMan, obj, pri) & 0xFF; // priority
	pSnd->volume = GET_SEL32V(_segMan, obj, vol) & 0xFF; // volume
	pSnd->signal = pSnd->dataInc = 0;

	_music->soundKill(pSnd);
	if (res)
		_music->soundInitSnd(res, pSnd);
	*/
#endif
}

void SoundCommandParser::cmdPlayHandle(reg_t obj, SongHandle handle, int value) {
	if (!obj.segment)
		return;

	_state->sfx_song_set_status(handle, SOUND_STATUS_PLAYING);
	_state->sfx_song_set_loops(handle, GET_SEL32V(_segMan, obj, loop));

	if (_doSoundVersion == SCI_VERSION_0_EARLY) {
		PUT_SEL32V(_segMan, obj, state, _K_SOUND_STATUS_PLAYING);
	} else if (_doSoundVersion == SCI_VERSION_1_EARLY) {
		_state->sfx_song_renice(handle, GET_SEL32V(_segMan, obj, pri));
		RESTORE_BEHAVIOR rb = (RESTORE_BEHAVIOR) value;		/* Too lazy to look up a default value for this */
		_state->_songlib.setSongRestoreBehavior(handle, rb);
		PUT_SEL32V(_segMan, obj, signal, 0);
	} else if (_doSoundVersion == SCI_VERSION_1_LATE) {
		int looping = GET_SEL32V(_segMan, obj, loop);
		//int vol = GET_SEL32V(_segMan, obj, vol);
		int pri = GET_SEL32V(_segMan, obj, pri);
		int sampleLen = 0;
		Song *song = _state->_songlib.findSong(handle);
		int songNumber = GET_SEL32V(_segMan, obj, number);

		if (GET_SEL32V(_segMan, obj, nodePtr) && (song && songNumber != song->_resourceNum)) {
			_state->sfx_song_set_status(handle, SOUND_STATUS_STOPPED);
			_state->sfx_remove_song(handle);
			PUT_SEL32(_segMan, obj, nodePtr, NULL_REG);
		}

		if (!GET_SEL32V(_segMan, obj, nodePtr) && obj.segment) {
			// In SCI1.1 games, sound effects are started from here. If we can find
			// a relevant audio resource, play it, otherwise switch to synthesized
			// effects. If the resource exists, play it using map 65535 (sound
			// effects map)
			if (_resMan->testResource(ResourceId(kResourceTypeAudio, songNumber)) &&
				getSciVersion() >= SCI_VERSION_1_1) {
				// Found a relevant audio resource, play it
				_audio->stopAudio();
				warning("Initializing audio resource instead of requested sound resource %d", songNumber);
				sampleLen = _audio->startAudio(65535, songNumber);
				// Also create iterator, that will fire SI_FINISHED event, when the sound is done playing
				_state->sfx_add_song(new_timer_iterator(sampleLen), 0, handle, songNumber);
			} else {
				if (!_resMan->testResource(ResourceId(kResourceTypeSound, songNumber))) {
					warning("Could not open song number %d", songNumber);
					// Send a "stop handle" event so that the engine won't wait forever here
					_state->sfx_song_set_status(handle, SOUND_STATUS_STOPPED);
					PUT_SEL32V(_segMan, obj, signal, SIGNAL_OFFSET);
					return;
				}
				debugC(2, kDebugLevelSound, "Initializing song number %d\n", songNumber);
				_state->sfx_add_song(build_iterator(_resMan, songNumber, SCI_SONG_ITERATOR_TYPE_SCI1,
				                          handle), 0, handle, songNumber);
			}

			PUT_SEL32(_segMan, obj, nodePtr, obj);
			PUT_SEL32(_segMan, obj, handle, obj);
		}

		if (obj.segment) {
			_state->sfx_song_set_status(handle, SOUND_STATUS_PLAYING);
			_state->sfx_song_set_loops(handle, looping);
			_state->sfx_song_renice(handle, pri);
			PUT_SEL32V(_segMan, obj, signal, 0);
		}
	}
}

void SoundCommandParser::cmdDummy(reg_t obj, SongHandle handle, int value) {
	warning("cmdDummy invoked");	// not supposed to occur
}

void SoundCommandParser::changeHandleStatus(reg_t obj, SongHandle handle, int newStatus) {
	if (obj.segment) {
		_state->sfx_song_set_status(handle, newStatus);
		if (_doSoundVersion == SCI_VERSION_0_EARLY)
			PUT_SEL32V(_segMan, obj, state, newStatus);
	}
}

void SoundCommandParser::cmdDisposeHandle(reg_t obj, SongHandle handle, int value) {
	changeHandleStatus(obj, handle, SOUND_STATUS_STOPPED);

	if (obj.segment) {
		_state->sfx_remove_song(handle);

		if (!_hasNodePtr)
			PUT_SEL32V(_segMan, obj, handle, 0x0000);
	}
}

void SoundCommandParser::cmdStopHandle(reg_t obj, SongHandle handle, int value) {
	changeHandleStatus(obj, handle, SOUND_STATUS_STOPPED);

	if (_hasNodePtr)
		PUT_SEL32V(_segMan, obj, signal, SIGNAL_OFFSET);
}

void SoundCommandParser::cmdSuspendHandle(reg_t obj, SongHandle handle, int value) {
	if (!_hasNodePtr)
		changeHandleStatus(obj, handle, SOUND_STATUS_SUSPENDED);
	else
		changeHandleStatus(obj, handle, value ? SOUND_STATUS_SUSPENDED : SOUND_STATUS_PLAYING);
}

void SoundCommandParser::cmdResumeHandle(reg_t obj, SongHandle handle, int value) {
	changeHandleStatus(obj, handle, SOUND_STATUS_PLAYING);
}

void SoundCommandParser::cmdMuteSound(reg_t obj, SongHandle handle, int value) {
	// TODO

	/* if there's a parameter, we're setting it.  Otherwise, we're querying it. */
	/*int param = UPARAM_OR_ALT(1,-1);

	if (param != -1)
	s->acc = s->sound_server->command(s, SOUND_COMMAND_SET_MUTE, 0, param);
	else
	s->acc = s->sound_server->command(s, SOUND_COMMAND_GET_MUTE, 0, 0);*/
}

void SoundCommandParser::cmdVolume(reg_t obj, SongHandle handle, int value) {
	if (obj != SIGNAL_REG)
		_state->sfx_setVolume(obj.toSint16());

	_acc = make_reg(0, _state->sfx_getVolume());
}

void SoundCommandParser::cmdHandlePriority(reg_t obj, SongHandle handle, int value) {
	if (obj.segment) {
		_state->sfx_song_set_loops(handle, GET_SEL32V(_segMan, obj, loop));
		script_set_priority(_resMan, _segMan, _state, obj, GET_SEL32V(_segMan, obj, pri));
	}
}

void SoundCommandParser::cmdFadeHandle(reg_t obj, SongHandle handle, int value) {
	/*s->sound_server->command(s, SOUND_COMMAND_FADE_HANDLE, obj, 120);*/ /* Fade out in 2 secs */
	/* FIXME: The next couple of lines actually STOP the handle, rather
	** than fading it! */
	if (obj.segment) {
		_state->sfx_song_set_status(handle, SOUND_STATUS_STOPPED);
		if (!_hasNodePtr)
			PUT_SEL32V(_segMan, obj, state, SOUND_STATUS_STOPPED);
		PUT_SEL32V(_segMan, obj, signal, SIGNAL_OFFSET);
	}

#if 0
		fade_params_t fade;
		if (obj.segment) {
			fade.final_volume = argv[2].toUint16();
			fade.ticks_per_step = argv[3].toUint16();
			fade.step_size = argv[4].toUint16();
			fade.action = argv[5].toUint16() ?
			              FADE_ACTION_FADE_AND_STOP :
			              FADE_ACTION_FADE_AND_CONT;

			s->_sound.sfx_song_set_fade(handle,  &fade);

			/* FIXME: The next couple of lines actually STOP the handle, rather
			** than fading it! */
			if (argv[5].toUint16()) {
				PUT_SEL32V(segMan, obj, signal, SIGNAL_OFFSET);
				s->_sound.sfx_song_set_status(handle, SOUND_STATUS_STOPPED);
			} else {
				// FIXME: Support fade-and-continue. For now, send signal right away.
				PUT_SEL32V(segMan, obj, signal, SIGNAL_OFFSET);
			}
		}
#endif
}

void SoundCommandParser::cmdGetPolyphony(reg_t obj, SongHandle handle, int value) {
	_acc = make_reg(0, _state->sfx_get_player_polyphony());
}

void cmdGetPlayNext(EngineState *s, reg_t obj, SongHandle handle, int value) {
	// TODO
	// _state->sfx_all_stop();
}

void SoundCommandParser::cmdUpdateHandle(reg_t obj, SongHandle handle, int value) {
	// FIXME: Get these from the sound server
	int signal = 0;
	int min = 0;
	int sec = 0;
	int frame = 0;

	// FIXME: Update the sound server state with 'vol'
	//int vol = GET_SEL32V(segMan, obj, vol);

	_state->sfx_song_set_loops(handle, GET_SEL32V(_segMan, obj, loop));
	_state->sfx_song_renice(handle, GET_SEL32V(_segMan, obj, pri));

	debugC(2, kDebugLevelSound, "[sound01-update-handle] -- CUE %04x:%04x", PRINT_REG(obj));

	PUT_SEL32V(_segMan, obj, signal, signal);
	PUT_SEL32V(_segMan, obj, min, min);
	PUT_SEL32V(_segMan, obj, sec, sec);
	PUT_SEL32V(_segMan, obj, frame, frame);
}

void SoundCommandParser::cmdUpdateCues(reg_t obj, SongHandle handle, int value) {
	int signal = 0;
	int min = 0;
	int sec = 0;
	int frame = 0;
	int result = SI_LOOP; // small hack
	int cue = 0;

	while (result == SI_LOOP)
		result = _state->sfx_poll_specific(handle, &cue);

	switch (result) {
	case SI_ABSOLUTE_CUE:
		signal = cue;
		debugC(2, kDebugLevelSound, "---    [CUE] %04x:%04x Absolute Cue: %d\n",
		          PRINT_REG(obj), signal);

		PUT_SEL32V(_segMan, obj, signal, signal);
		break;

	case SI_RELATIVE_CUE:
		signal = cue;
		debugC(2, kDebugLevelSound, "---    [CUE] %04x:%04x Relative Cue: %d\n",
		          PRINT_REG(obj), cue);

		/* FIXME to match commented-out semantics
		 * below, with proper storage of dataInc and
		 * signal in the iterator code. */
		PUT_SEL32V(_segMan, obj, dataInc, signal);
		PUT_SEL32V(_segMan, obj, signal, signal);
		break;

	case SI_FINISHED:
		debugC(2, kDebugLevelSound, "---    [FINISHED] %04x:%04x\n", PRINT_REG(obj));
		PUT_SEL32V(_segMan, obj, signal, SIGNAL_OFFSET);
		break;

	case SI_LOOP:
		break; // Doesn't happen
	}

	//switch (signal) {
	//case 0x00:
	//	if (dataInc!=GET_SEL32V(segMan, obj, dataInc)) {
	//		PUT_SEL32V(segMan, obj, dataInc, dataInc);
	//		PUT_SEL32V(segMan, obj, signal, dataInc+0x7f);
	//	} else {
	//		PUT_SEL32V(segMan, obj, signal, signal);
	//	}
	//	break;
	//case 0xFF: // May be unnecessary
	//	s->_sound.sfx_song_set_status(handle, SOUND_STATUS_STOPPED);
	//	break;
	//default :
	//	if (dataInc!=GET_SEL32V(segMan, obj, dataInc)) {
	//		PUT_SEL32V(segMan, obj, dataInc, dataInc);
	//		PUT_SEL32V(segMan, obj, signal, dataInc + 0x7f);
	//	} else {
	//		PUT_SEL32V(segMan, obj, signal, signal);
	//	}
	//	break;
	//}

	PUT_SEL32V(_segMan, obj, min, min);
	PUT_SEL32V(_segMan, obj, sec, sec);
	PUT_SEL32V(_segMan, obj, frame, frame);
}

void SoundCommandParser::cmdSendMidi(reg_t obj, SongHandle handle, int value) {
	int channel = value;

	_state->sfx_send_midi(handle, channel, _midiCmd, _controller, _param);
}

void SoundCommandParser::cmdReverb(reg_t obj, SongHandle handle, int value) {
	// TODO
}
void SoundCommandParser::cmdHoldHandle(reg_t obj, SongHandle handle, int value) {
	_state->sfx_song_set_hold(handle, value);
}

void SoundCommandParser::cmdGetAudioCapability(reg_t obj, SongHandle handle, int value) {
	// Tests for digital audio support
	_acc = make_reg(0, 1);
}

void SoundCommandParser::cmdGetPlayNext(reg_t obj, SongHandle handle, int value) {
}

void SoundCommandParser::cmdSetHandleVolume(reg_t obj, SongHandle handle, int value) {
	// TODO
}

void SoundCommandParser::cmdSetHandlePriority(reg_t obj, SongHandle handle, int value) {
	script_set_priority(_resMan, _segMan, _state, obj, value);
}

void SoundCommandParser::cmdSetHandleLoop(reg_t obj, SongHandle handle, int value) {
	if (!GET_SEL32(_segMan, obj, nodePtr).isNull()) {
		uint16 looping = value;

		if (looping < 65535)
			looping = 1;

		_state->sfx_song_set_loops(handle, looping);
		PUT_SEL32V(_segMan, obj, loop, looping);
	}
}

void SoundCommandParser::cmdSuspendSound(reg_t obj, SongHandle handle, int value) {
	// TODO
}

void SoundCommandParser::cmdUpdateVolumePriority(reg_t obj, SongHandle handle, int value) {
	// TODO
}

} // End of namespace Sci
