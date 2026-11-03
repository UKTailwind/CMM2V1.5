# Appendix A – The Services

The services in the order of the table (struct cmm2_services, in Inc/cmm2img.h), with the version of the firmware's services that added each and the chapter that describes it. The table starts with two numbers: version, the firmware's version of the services (7 in 7.00.00RC0), and size, the table's size in bytes.

{widths: 9,46,9,36}
| Ver | Service | Ver | Service |
|---|---|---|---|
| 1 | `void con_putc(int c)` | 1 | `void *alloc(uint32_t size)` |
| 1 | `void con_puts(const char *s)` | 1 | `void release(void *p)` |
| 1 | `int con_getc(void)` | 1 | `void *opl_new(uint32_t rate)` |
| 1 | `uint64_t micros(void)` | 1 | `void opl_write(chip, reg, value)` |
| 1 | `void delay_us(uint32_t us)` | 1 | `void opl_generate(chip, buf, frames)` |
| 1 | `void datetime(int t[6])` | 1 | `void opl_free(void *chip)` |
| 1 | `int file_open(name, mode)` | 2 | `int video_mode(mode, depth, v)` |
| 1 | `int file_close(int h)` | 2 | `int video_palette(start, count, rgb)` |
| 1 | `int32_t file_read(h, buf, n)` | 2 | `void video_wait(void)` |
| 1 | `int32_t file_write(h, buf, n)` | 2 | `int keys_held(int keys[7])` |
| 1 | `int file_seek(int h, uint32_t pos)` | 3 | `int audio_start(fill)` |
| 1 | `int32_t file_tell(int h)` | 3 | `void audio_stop(void)` |
| 1 | `int32_t file_size(int h)` | 3 | `void audio_lock(void)` |
| 1 | `int file_sync(int h)` | 3 | `void audio_unlock(void)` |
| 1 | `int file_remove(const char *name)` | 3 | `int music_play(midi, size, loop, bank)` |
| 1 | `int file_rename(from, to)` | 3 | `void music_stop(void)` |
| 1 | `int dir_make(const char *name)` | 3 | `void music_volume(int volume)` |
| 1 | `int dir_open(const char *path)` | 3 | `void music_pause(int paused)` |
| 1 | `int dir_read(int h, cmm2_dirent *e)` | 3 | `int music_playing(void)` |
| 1 | `int dir_close(int h)` | 3 | `void music_rate(int divisor)` |
| 1 | `int poll(void)` | 4 | `int mouse(int m[4])` |
| 1 | `void leave(int code)` | 4 | `void mouse_set(int x, int y)` |
| 1 | `uint32_t cpu_hz(void)` | 4 | `void con_screen(int on)` |

{widths: 9,46,9,36}
| Ver | Service | Ver | Service |
|---|---|---|---|
| 5 | `int break_take(void)` | 7 | `int i2c_open(bus, khz, timeout_ms)` |
| 5 | `int file_stat(name, cmm2_stat *st)` | 7 | `int i2c_close(int bus)` |
| 6 | `int video_scaled(mode, depth, scale, v)` | 7 | `int i2c_xfer(bus, addr, w, wn, r, rn, stop)` |
| 6 | `void frame_hook(void (*fn)(void))` | 7 | `int spi_open(bus, hz, mode)` |
| 6 | `int dma2d_copy(src, sp, dst, dp, w, h)` | 7 | `int spi_close(int bus)` |
| 6 | `int dma2d_busy(void)` | 7 | `int spi_xfer(bus, tx, rx, n)` |
| 7 | `int pin_setup(gp, mode, pull)` | 7 | `int com_open(com, baud, bits, parity, stop, rxbuf)` |
| 7 | `int pin_read(int gp)` | 7 | `int com_close(int com)` |
| 7 | `void pin_write(int gp, int v)` | 7 | `int com_write(com, buf, n)` |
| 7 | `int adc_read(int gp, uint32_t *uv)` | 7 | `int com_read(com, buf, n)` |
| 7 | `int pwm_set(gp, freq, duty)` | 7 | `int com_avail(int com)` |
| 7 | `int pwm_stop(int gp)` | 7 | `const char *io_error(void)` |

The console, the clock, files, directories, the system and the FM synthesiser are in the chapter The Services; the screen in The Screen; the keys and mouse in The Keyboard and Mouse; audio and music in Sound and Music; and the I/O services in The I/O Pins. Inc/cmm2img.h has the full declarations, with a comment on each.
