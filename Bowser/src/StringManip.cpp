
#include <String.h>

#include <stdio.h>

#include "Bowser.h"

BString
GetWord (const char *cData, int32 wordNeeded)
{
	BString data (cData);
	BString buffer ("-9z99");
	int32 wordAt (1), place (0);

	while (wordAt != wordNeeded && place != B_ERROR)
	{
		if ((place = data.FindFirst ('\x20', place)) != B_ERROR)
			if (++place < data.Length()
			&&  data[place] != '\x20')
				++wordAt;
	}

	if (wordAt == wordNeeded
	&&  place != B_ERROR
	&&  place < data.Length())
	{
		int32 end (data.FindFirst ('\x20', place));

		if (end == B_ERROR)
			end = data.Length();

		data.CopyInto (buffer, place, end - place);
	}

	return buffer;
}

BString
RestOfString (const char *cData, int32 wordStart)
{
	BString data (cData);
	int32 wordAt (1), place (0);
	BString buffer ("-9z99");
	
	while (wordAt != wordStart && place != B_ERROR)
	{
		if ((place = data.FindFirst ('\x20', place)) != B_ERROR)
			if (++place < data.Length()
			&&  data[place] != '\x20')
				++wordAt;
	}

	if (wordAt == wordStart
	&&  place != B_ERROR
	&&  place < data.Length())
		data.CopyInto (buffer, place, data.Length() - place);

	return buffer;
}

BString
GetNick (const char *cData)
{
	BString data (cData);
	BString theNick;

	for (int32 i = 1;
			i < data.Length() && data[i] != '!' && data[i] != '\x20';
			++i)
		theNick << data[i];

	return theNick;
}

BString
GetIdent (const char *cData)
{
	BString data (GetWord(cData, 1));
	BString theIdent;
	int32 place[2];

	if ((place[0] = data.FindFirst ('!')) != B_ERROR
	&&  (place[1] = data.FindFirst ('@')) != B_ERROR)
	{
		++(place[0]);
		data.CopyInto (theIdent, place[0], place[1] - place[0]);
	}
		
	return theIdent;
}

BString
GetAddress (const char *cData)
{
	BString data (GetWord(cData, 1));
	BString address;
	int32 place;

	if ((place = data.FindFirst ('@')) != B_ERROR)
	{
		int32 length (data.FindFirst ('\x20', place));

		if (length == B_ERROR)
			length = data.Length();

		++place;
		data.CopyInto (address, place, length - place);
	}

	return address;
}

BString
TimeStamp()
{
	if(!bowser_app->GetStampState())
		return "";

	time_t myTime (time (0));
	tm curTime = *localtime (&myTime);
	
	char tempTime[32];
	sprintf(tempTime, "[%02d:%02d] ", curTime.tm_hour, curTime.tm_min);
	
	return BString(tempTime);
}


BString
ExpandKeyed (
	const char *incoming,
	const char *keys,
	const char **expansions)
{
	BString buffer;

	while (incoming && *incoming)
	{
		if (*incoming == '$')
		{
			const char *place;

			++incoming;

			if ((place = strchr (keys, *incoming)) != 0)
				buffer << expansions[place - keys];
			else
				buffer << *incoming;
		}
		else
			buffer << *incoming;

		++incoming;
	}

	buffer << "\n";

	return buffer;
}
