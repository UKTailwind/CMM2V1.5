# curl

curl fetches files and pages from the internet over HTTP and HTTPS, and sends forms and data to web servers. It is the same curl that is on almost every other computer, version 8.23.0, with the same options, so the examples you find on the web work here too.

It needs firmware 8.00.00b2 or later, the Wi-Fi module, and OPTION WIFI set so that the Colour Maximite 2 has joined your network. MM.INFO(IP ADDRESS) shows whether it has.

## Installing It

Copy the curl folder from the programs zip to the root of the SD card. It holds curl.app and cacert.pem, the list of the certificate authorities that HTTPS sites are checked against.

## Starting It

```
RUN "A:/curl/curl.app", -o page.html https://example.com/
```

You can run it from any folder: a file it saves goes in the current folder unless you give a path. The words after the comma are curl's options and the address, as on any other computer.

A word with a space in it goes in double quotes:

```
RUN "A:/curl/curl.app", -H "Accept: text/plain" https://example.com/
```

A double quote cannot be passed in a word, and a command line can be at most about 250 characters. For anything longer, put the options in a file and use -K (see Options in a File, below).

## Leaving It

curl ends by itself when it has finished. **Ctrl-C** stops it, in the middle of a transfer or while it prints.

## Some Examples

{widths: 24,28}
| Command (after RUN "A:/curl/curl.app",) | What it does |
| https://example.com/ | Shows the page on the screen |
| -o page.html https://example.com/ | Saves it as page.html |
| -O https://example.com/files/data.zip | Saves it under its own name, data.zip |
| -L -O https://example.com/latest | Follows redirections to the file itself |
| -C - -O https://example.com/files/big.zip | Carries on with a download that was stopped |
| -I https://example.com/ | Shows only the headers |
| -d "name=Peter&city=Perth" https://example.com/form | Sends a form (POST) |
| -F "file=@photo.jpg" https://example.com/upload | Sends a file as a form does |
| -H "Content-Type: application/json" -d @data.json https://example.com/api | Sends JSON from a file |
| -u user:password https://example.com/private/ | Logs in with a user name and password |
| -c cookies.txt -b cookies.txt https://example.com/ | Keeps cookies from one run to the next |
| -sS --compressed -o page.html https://example.com/ | Quietly, with the page sent compressed |

-s turns off the progress meter, and -S still shows errors when -s is on. --compressed asks the server to send gzip, which is often less than a third of the size.

-H @file reads headers from a file, one to a line, instead of from the command line.

## Options in a File

-K reads options from a file, one to a line, without the leading dashes. Quotes, long headers and lists of files all go in it. Make it with the editor, or have a program write it with PRINT #:

```
# A:/fetch.cfg
silent
show-error
header = "User-Agent: Colour Maximite 2"
url = "https://example.com/one.txt"
output = "one.txt"
url = "https://example.com/two.txt"
output = "two.txt"
```

```
RUN "A:/curl/curl.app", -K A:/fetch.cfg
```

Add the line parallel to fetch the files at the same time, which is quicker when there are several. A line that starts with # is a comment.

## Help

-h shows the commonest options. --help all lists every one, and --manual shows curl's whole manual (it is long: Ctrl-C stops it). curl's manual on the web, https://curl.se/docs/manpage.html, is the same.

## Certificates and the Clock

For an HTTPS address curl checks that the site's certificate was signed by one of the authorities in cacert.pem, and refuses the site if not. It looks for cacert.pem in the folder curl.app is in, then for A:/cacert.pem. --cacert gives another file.

**The clock must be right.** A certificate is valid only between two dates, so with the wrong date every HTTPS site is refused. curl then says what date the Colour Maximite 2 thinks it is. WEB NTP sets the clock from the internet, and a DS3231 keeps it.

cacert.pem goes out of date as authorities come and go. curl can fetch a new one itself, from the people who make curl:

```
RUN "A:/curl/curl.app", -o A:/curl/cacert.new https://curl.se/ca/cacert.pem
```

then RENAME it over the old one when it has arrived. -k turns the checking off, for a site with a home-made certificate on your own network: never use it for anything that matters.

## Files on the Card: file: Addresses

curl reads a file on the card with a file: address, such as file:///curl/notes.txt. It must not have a drive letter (curl accepts those only on Windows, and refuses file:///A:/notes.txt with code 3): the file is on the current drive.

## When It Fails

If curl stops with an error, it says why, and MMBasic gives its number: A:/curl/curl.app ended with code n. The commonest are:

{widths: 10,42}
| Code | What it means |
| 3 | The address is not one curl can use, such as a file: address with a drive letter |
| 6 | The site's name was not found. Check the spelling and that Wi-Fi is up |
| 7 | The site did not answer |
| 22 | The server said no (with -f), such as 404, not found |
| 23 | The file could not be written: a wrong folder, or the card is full |
| 26 | A file to send (-F or -T) could not be read |
| 28 | It took too long |
| 35 | The secure connection could not be made |
| 60 | The site's certificate was refused: see Certificates and the Clock |
| 77 | cacert.pem could not be read: it is not beside curl.app or in A:/ |
| 130 | Ctrl-C was pressed |

Some sites refuse anything that is not a full web browser, and send a page saying so, or 403. curl cannot get past those.

## What It Does Not Do

- Only HTTP, HTTPS and file: addresses. FTP, SFTP, SCP, email and the other protocols curl has elsewhere are left out.
- HTTP/1.1 only, which every web server still speaks. Not HTTP/2 or HTTP/3.
- No IPv6, and no international domain names (names with letters other than a to z).
- Compression: gzip and deflate, not Brotli or zstd. Servers send gzip when that is all curl asks for.
- No .curlrc file read at the start, and --remote-time cannot set a file's date.
- No drive letter in a file: address.

## Speed

Over HTTPS curl downloads at up to about 1MB a second, less when the Wi-Fi signal is weak. Connecting to a new site over HTTPS takes about a second, most of it reading cacert.pem. Saved to the SD card a download goes no faster than the card can write, which on a slow card is 150KB a second or less.
