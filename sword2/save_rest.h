/* Copyright (C) 1994-1998 Revolution Software Ltd.
 * Copyright (C) 2003-2005 The ScummVM project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header$
 */

#ifndef	SAVE_REST_H
#define	SAVE_REST_H

namespace Sword2 {

#define	SAVE_DESCRIPTION_LEN	64

// Save & Restore error codes

enum {
	SR_OK,			// No worries
	SR_ERR_FILEOPEN,	// Can't open file - Couldn't create file for
				// saving, or couldn't find file for loading.
	SR_ERR_INCOMPATIBLE,	// (Restore) Incompatible savegame data.
				// Savegame file is obsolete. (Won't happen
				// after development stops)
	SR_ERR_READFAIL,	// (Restore) Failed on reading savegame file -
				// Something screwed up during the read
	SR_ERR_WRITEFAIL	// (Save) Failed on writing savegame file -
				// Something screwed up during the write -
				// could be hard-drive full..?
};

} // End of namespace Sword2

#endif
