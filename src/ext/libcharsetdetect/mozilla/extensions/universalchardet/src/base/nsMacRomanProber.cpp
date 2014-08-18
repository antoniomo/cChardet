/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Universal charset detector code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *          Antonio Macías <antonio.macias.ojeda@gmail.com> - Backport to CPP
 *          Rob Speer - adapt to MacRoman encoding in Python (chardet)
 *          Shy Shalom <shooshX@gmail.com> - Original Latin1 prober
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsMacRomanProber.h"
#include "prmem.h"
#include <stdio.h>

#define UDF    0        // undefined
#define OTH    1        // other
#define ASC    2        // ascii capital letter
#define ASS    3        // ascii small letter
#define ACV    4        // accent capital vowel
#define ACO    5        // accent capital other
#define ASV    6        // accent small vowel
#define ASO    7        // accent small other
#define ODD    8        // character that is unlikely to appear
#define CLASS_NUM   9   // total classes

/* The change from Latin1 is that we explicitly look for extended characters
that are infrequently-occurring symbols, and consider them to always be
improbable. This should let MacRoman get out of the way of more likely
encodings in most situations.
*/

static const unsigned char MacRoman_CharToClass[] =
{
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,   // 00 - 07
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,   // 08 - 0F
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,   // 10 - 17
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,   // 18 - 1F
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,   // 20 - 27
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,   // 28 - 2F
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,   // 30 - 37
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,   // 38 - 3F
  OTH, ASC, ASC, ASC, ASC, ASC, ASC, ASC,   // 40 - 47
  ASC, ASC, ASC, ASC, ASC, ASC, ASC, ASC,   // 48 - 4F
  ASC, ASC, ASC, ASC, ASC, ASC, ASC, ASC,   // 50 - 57
  ASC, ASC, ASC, OTH, OTH, OTH, OTH, OTH,   // 58 - 5F
  OTH, ASS, ASS, ASS, ASS, ASS, ASS, ASS,   // 60 - 67
  ASS, ASS, ASS, ASS, ASS, ASS, ASS, ASS,   // 68 - 6F
  ASS, ASS, ASS, ASS, ASS, ASS, ASS, ASS,   // 70 - 77
  ASS, ASS, ASS, OTH, OTH, OTH, OTH, OTH,   // 78 - 7F
  ACV, ACV, ACO, ACV, ACO, ACV, ACV, ASV,   // 80 - 87
  ASV, ASV, ASV, ASV, ASV, ASO, ASV, ASV,   // 88 - 8F
  ASV, ASV, ASV, ASV, ASV, ASV, ASO, ASV,   // 90 - 97
  ASV, ASV, ASV, ASV, ASV, ASV, ASV, ASV,   // 98 - 9F
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, ASO,   // A0 - A7
  OTH, OTH, ODD, ODD, OTH, OTH, ACV, ACV,   // A8 - AF
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH,   // B0 - B7
  OTH, OTH, OTH, OTH, OTH, OTH, ASV, ASV,   // B8 - BF
  OTH, OTH, ODD, OTH, ODD, OTH, OTH, OTH,   // C0 - C7
  OTH, OTH, OTH, ACV, ACV, ACV, ACV, ASV,   // C8 - CF
  OTH, OTH, OTH, OTH, OTH, OTH, OTH, ODD,   // D0 - D7
  ASV, ACV, ODD, OTH, OTH, OTH, OTH, OTH,   // D8 - DF
  OTH, OTH, OTH, OTH, OTH, ACV, ACV, ACV,   // E0 - E7
  ACV, ACV, ACV, ACV, ACV, ACV, ACV, ACV,   // E8 - EF
  ODD, ACV, ACV, ACV, ACV, ASV, ODD, ODD,   // F0 - F7
  ODD, ODD, ODD, ODD, ODD, ODD, ODD, ODD,   // F8 - FF
};


/* 0 : illegal
   1 : very unlikely
   2 : normal
   3 : very likely
*/
static const unsigned char MacRomanClassModel[] =
{
/*      UDF OTH ASC ASS ACV ACO ASV ASO ODD */
/*UDF*/  0,  0,  0,  0,  0,  0,  0,  0,  0,
/*OTH*/  0,  3,  3,  3,  3,  3,  3,  3,  1,
/*ASC*/  0,  3,  3,  3,  3,  3,  3,  3,  1,
/*ASS*/  0,  3,  3,  3,  1,  1,  3,  3,  1,
/*ACV*/  0,  3,  3,  3,  1,  2,  1,  2,  1,
/*ACO*/  0,  3,  3,  3,  3,  3,  3,  3,  1,
/*ASV*/  0,  3,  1,  3,  1,  1,  1,  3,  1,
/*ASO*/  0,  3,  1,  3,  1,  1,  3,  3,  1,
/*ODD*/  0,  1,  1,  1,  1,  1,  1,  1,  1,
};

void  nsMacRomanProber::Reset(void)
{
  mState = eDetecting;
  mLastCharClass = OTH;
  for (int i = 0; i < FREQ_CAT_NUM; i++)
    mFreqCounter[i] = 0;
}


nsProbingState nsMacRomanProber::HandleData(const char* aBuf, PRUint32 aLen)
{
  char *newBuf1 = 0;
  PRUint32 newLen1 = 0;

  if (!FilterWithEnglishLetters(aBuf, aLen, &newBuf1, newLen1)) {
    newBuf1 = (char*)aBuf;
    newLen1 = aLen;
  }

  unsigned char charClass;
  unsigned char freq;
  for (PRUint32 i = 0; i < newLen1; i++)
  {
    charClass = MacRoman_CharToClass[(unsigned char)newBuf1[i]];
    freq = MacRomanClassModel[mLastCharClass*CLASS_NUM + charClass];
    if (freq == 0) {
      mState = eNotMe;
      break;
    }
    mFreqCounter[freq]++;
    mLastCharClass = charClass;
  }

  if (newBuf1 != aBuf)
    PR_FREEIF(newBuf1);

  return mState;
}

float nsMacRomanProber::GetConfidence(void)
{
  if (mState == eNotMe)
    return 0.01f;

  float confidence;
  PRUint32 total = 0;
  for (PRInt32 i = 0; i < FREQ_CAT_NUM; i++)
    total += mFreqCounter[i];

  if(!total)
    confidence = 0.0f;
  else
  {
    confidence = mFreqCounter[3]*1.0f / total;
    confidence -= mFreqCounter[1]*20.0f/total;
  }

  if (confidence < 0.0f)
    confidence = 0.0f;

  // lower the confidence of MacRoman so that other more accurate detector
  // can take priority.
  confidence *= 0.50f;

  return confidence;
}

#ifdef DEBUG_chardet
void  nsMacRomanProber::DumpStatus()
{
  printf(" MacRomanProber: %1.3f [%s]\r\n", GetConfidence(), GetCharSetName());
}
#endif
