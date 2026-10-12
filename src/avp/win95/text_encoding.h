#ifndef AVP_TEXT_ENCODING_H
#define AVP_TEXT_ENCODING_H

/*
 * The menu/HUD font texture contains the ASCII page.  Text may arrive either
 * as UTF-8 or as legacy Windows-1252 bytes.  Decode one character at a time,
 * return its ASCII base glyph and optionally the ASCII mark used to compose
 * a Western-Latin diacritic in renderers that support the overlay.
 */
static inline unsigned char AvPTextLatinGlyph(unsigned int codepoint, unsigned char *accent)
{
	if (accent) *accent = 0;

	switch (codepoint)
	{
		case 0x00c0: case 0x00e0: if (accent) *accent = '`'; return (codepoint < 0x00e0) ? 'A' : 'a';
		case 0x00c1: case 0x00e1: case 0x00c9: case 0x00e9: case 0x00cd: case 0x00ed:
		case 0x00d3: case 0x00f3: case 0x00da: case 0x00fa: case 0x00dd: case 0x00fd:
			if (accent) *accent = '\'';
			break;
		case 0x00c2: case 0x00e2: case 0x00ca: case 0x00ea: case 0x00ce: case 0x00ee:
		case 0x00d4: case 0x00f4: case 0x00db: case 0x00fb:
			if (accent) *accent = '^';
			break;
		case 0x00c3: case 0x00e3: case 0x00d1: case 0x00f1: case 0x00d5: case 0x00f5:
			if (accent) *accent = '~';
			break;
		case 0x00c4: case 0x00e4: case 0x00cb: case 0x00eb: case 0x00cf: case 0x00ef:
		case 0x00d6: case 0x00f6: case 0x00dc: case 0x00fc: case 0x00ff:
			if (accent) *accent = '"';
			break;
		case 0x00c7: case 0x00e7: if (accent) *accent = ','; return (codepoint == 0x00c7) ? 'C' : 'c';
		case 0x00d8: case 0x00f8: if (accent) *accent = '/'; return (codepoint == 0x00d8) ? 'O' : 'o';
		case 0x00c5: case 0x00e5: if (accent) *accent = 'o'; return (codepoint == 0x00c5) ? 'A' : 'a';
		case 0x00c6: case 0x00e6: return (codepoint == 0x00c6) ? 'A' : 'a';
		case 0x00d0: return 'D'; case 0x00f0: return 'd'; case 0x00de: return 'P'; case 0x00fe: return 'p'; case 0x00df: return 'B';
		case 0x00a0: return ' '; case 0x00a1: return '!'; case 0x00ab: case 0x00bb: return '"'; case 0x00ad: return '-'; case 0x00bf: return '?';
		case 0x20ac: return '$'; case 0x2018: case 0x2019: return '\''; case 0x201c: case 0x201d: return '"'; case 0x2013: case 0x2014: return '-'; case 0x2026: return '.';
		default: break;
	}

	/* The accent groups above share this base-letter mapping. */
	switch (codepoint)
	{
		case 0x00c1: case 0x00c2: case 0x00c3: case 0x00c4: return 'A';
		case 0x00e1: case 0x00e2: case 0x00e3: case 0x00e4: return 'a';
		case 0x00c8: case 0x00c9: case 0x00ca: case 0x00cb: return 'E';
		case 0x00e8: case 0x00e9: case 0x00ea: case 0x00eb: return 'e';
		case 0x00cc: case 0x00cd: case 0x00ce: case 0x00cf: return 'I';
		case 0x00ec: case 0x00ed: case 0x00ee: case 0x00ef: return 'i';
		case 0x00d1: return 'N'; case 0x00f1: return 'n';
		case 0x00d2: case 0x00d3: case 0x00d4: case 0x00d5: case 0x00d6: return 'O';
		case 0x00f2: case 0x00f3: case 0x00f4: case 0x00f5: case 0x00f6: return 'o';
		case 0x00d9: case 0x00da: case 0x00db: case 0x00dc: return 'U';
		case 0x00f9: case 0x00fa: case 0x00fb: case 0x00fc: return 'u';
		case 0x00dd: return 'Y'; case 0x00fd: case 0x00ff: return 'y';
		default: return (codepoint <= 0x7fU) ? (unsigned char)codepoint : '?';
	}
}

static inline unsigned char AvPTextNextGlyphConstWithAccent(const char **textPtr, unsigned char *accent)
{
	const unsigned char *p = (const unsigned char *)*textPtr;
	unsigned int codepoint;

	if (p[0] < 0x80U)
	{
		if (accent) *accent = 0;
		*textPtr = (const char *)(p + 1);
		return p[0];
	}
	if ((p[0] >= 0xc2U) && (p[0] <= 0xdfU) && ((p[1] & 0xc0U) == 0x80U))
	{
		codepoint = ((unsigned int)(p[0] & 0x1fU) << 6) | (unsigned int)(p[1] & 0x3fU);
		*textPtr = (const char *)(p + 2);
		return AvPTextLatinGlyph(codepoint, accent);
	}
	if ((p[0] >= 0xe0U) && (p[0] <= 0xefU) && ((p[1] & 0xc0U) == 0x80U) && ((p[2] & 0xc0U) == 0x80U))
	{
		codepoint = ((unsigned int)(p[0] & 0x0fU) << 12) | ((unsigned int)(p[1] & 0x3fU) << 6) | (unsigned int)(p[2] & 0x3fU);
		*textPtr = (const char *)(p + 3);
		return AvPTextLatinGlyph(codepoint, accent);
	}
	if ((p[0] >= 0xf0U) && (p[0] <= 0xf4U) && ((p[1] & 0xc0U) == 0x80U) && ((p[2] & 0xc0U) == 0x80U) && ((p[3] & 0xc0U) == 0x80U))
	{
		*textPtr = (const char *)(p + 4);
		if (accent) *accent = 0;
		return '?';
	}

	/* Single-byte text from older language files remains supported. */
	*textPtr = (const char *)(p + 1);
	return AvPTextLatinGlyph(p[0], accent);
}

static inline unsigned char AvPTextNextGlyphConst(const char **textPtr)
{
	return AvPTextNextGlyphConstWithAccent(textPtr, 0);
}

static inline unsigned char AvPTextNextGlyphWithAccent(char **textPtr, unsigned char *accent)
{
	const char *cursor = *textPtr;
	unsigned char glyph = AvPTextNextGlyphConstWithAccent(&cursor, accent);
	*textPtr = (char *)cursor;
	return glyph;
}

static inline unsigned char AvPTextNextGlyph(char **textPtr)
{
	return AvPTextNextGlyphWithAccent(textPtr, 0);
}

static inline int AvPTextStringWidth(const char *textPtr, const char *fontWidths)
{
	int width = 0;
	while (*textPtr)
	{
		unsigned char glyph = AvPTextNextGlyphConst(&textPtr);
		width += (unsigned char)fontWidths[glyph];
	}
	return width;
}

#endif /* AVP_TEXT_ENCODING_H */