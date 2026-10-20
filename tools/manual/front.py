"""front.py: the manual's front matter, About to This Manual (docs/Manual_Plan.md, M-D3). The b14 manual's text for
the Colour Maximite 2, with Gerry Allardice's 6.00.00 work and the 7.00 update credited and the libraries this build
uses. Paragraphs are found by the start of their text, so it can be run again after the text below is edited; the
list of acknowledgments is replaced whole."""
import copy, os, sys
import docx
from docx.oxml.ns import qn
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from build_skeleton import set_text, ptext, TODO, OUT

ABOUT = [
    'The Colour Maximite 2 was conceived and developed by Peter Mather (matherp on the Back Shed Forum) who also led '
    'the development project. It is based on the Colour Maximite developed by Geoff Graham and uses the MMBasic '
    'interpreter written by Geoff Graham ( http://geoffg.net ).',
    'A team of people from around the world assisted with testing, advice and developing some initial games and '
    'programs. These are Phil Boyce, Jim Hiley, Graeme Rixon, Robert Severson and Mauro Xavier.',
    'Version 6.00.00 of the firmware was developed by Gerry Allardice. Version 7.00 brings the MMBasic of the '
    'PicoMite version 7 to the Colour Maximite 2.',
]
SUPPORT = ('Support questions should be raised on the Back Shed forum ( http://www.thebackshed.com/forum/Microcontrollers ) '
           'where there are many enthusiastic Maximite, Micromite and PicoMite users who would be only too happy to '
           'help. The developers of both the Colour Maximite 2 and MMBasic are also regulars on this forum.')
THANKS = [
    'The Colour Maximite 2 firmware and MMBasic is copyright 2011-2026 by Geoff Graham, Peter Mather and Gerry Allardice.',
    '1-Wire Support is copyright 1999-2006 Dallas Semiconductor Corporation and 2012 Gerard Sexton.',
    'FatFs (SD card and USB drive) driver is copyright ChaN.',
    'TinyUSB (the USB host) is copyright tinyusb.org.',
    'MOD file support was written by Jean François DEL NERO (hxcmod.c).',
    'WAV, MP3, and FLAC file support are copyright David Reid.',
    'JPG support is thanks to Rich Geldreich.',
    'PNG support is copyright 2005-2010 Lode Vandevenne and 2010 Sean Middleditch.',
    'Marcel Rodrigues wrote the GIF decoder.',
    'Maury Quijada wrote the image resize and image rotate code.',
    'cJSON is copyright Dave Gamble and cJSON contributors.',
    'The CRC calculations are copyright Rob Tillaart.',
    'Thomas Williams and Gerry Allardice for MMBasic enhancements.',
    'STM32 drivers and software components are copyright STMicroelectronics.',
]
LICENCE = ('The compiled object code (the .bin file) for the Colour Maximite 2 is free software: you can use or '
           'redistribute it as you please. The source code is available via subscription (free of charge) to '
           'individuals for personal use or under a negotiated license for commercial use. In both cases go to '
           'http://mmbasic.com for details.')
MANUAL = ('The author of this manual is Geoff Graham. Version 6.00.00 updates are by Gerry Allardice and version 7.00 '
          'updates by Peter Mather, using the PicoMite manual for the features the two computers share. It is '
          'distributed under a Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Australia license '
          '(CC BY-NC-SA 3.0).')


def main():
    d = docx.Document(OUT)
    body = d.element.body
    els = list(body.iterchildren())
    end = next(i for i, e in enumerate(els) if ptext(e) == 'Contents')
    front = els[:end]
    find = lambda start: next(e for e in front if ptext(e).startswith(start))
    for e in front:                                     # the TODO note under About
        if ptext(e).startswith(TODO.strip()):
            body.remove(e)
    about = find('Peter Mather (matherp') if any(ptext(e).startswith('Peter Mather (matherp') for e in front) \
        else find('The Colour Maximite 2 was conceived')
    for e in [x for x in front if ptext(x) in ABOUT[1:]]:   # (a run again: the paragraphs added before)
        body.remove(e)
    set_text(about, ABOUT[0])
    prev = about
    for t in ABOUT[1:]:
        p = copy.deepcopy(about)
        set_text(p, t)
        prev.addnext(p)
        prev = p
    set_text(find('Support questions'), SUPPORT)
    # the acknowledgments: every paragraph with text between their heading and the licence, replaced whole
    head = find('Copyright and Acknowledgments')
    lic = next(e for e in front if ptext(e).startswith('The compiled object code'))
    i0, i1 = list(body).index(head) + 1, list(body).index(lic)
    old = [e for e in list(body)[i0:i1] if ptext(e)]
    template = copy.deepcopy(old[0])
    for e in old:
        body.remove(e)
    for t in THANKS:                                    # (each before the licence: in order)
        p = copy.deepcopy(template)
        set_text(p, t)
        lic.addprevious(p)
    set_text(lic, LICENCE)
    set_text(find('The author of this manual'), MANUAL)
    d.save(OUT)
    print('front matter written')


if __name__ == '__main__':
    main()
