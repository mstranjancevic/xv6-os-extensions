# xv6 Operating System Extensions

Two kernel extensions written in C for the Operating Systems course at the Faculty of Computing (RAF), Belgrade. Each assignment is a standalone build of MIT's 32-bit x86 xv6 teaching operating system.

## Assignment 1 · Interactive console search

Implemented incremental, case-insensitive search over the text visible on the xv6 screen. The kernel console renders a search prompt on the bottom row, highlights matches as the query changes, and uses a distinct highlight for the selected match. Closing search restores the saved screen and cursor position.

The keyboard driver decodes the Alt shortcuts from scan codes. The console handles query editing and match navigation and updates the VGA text buffer directly. [Keyboard driver](assignment-1/kernel/kbd.c) · [Console implementation](assignment-1/kernel/console.c)

| Shortcut | Action |
| --- | --- |
| `Alt+F` | Open or close search |
| `Backspace` | Delete the last query character |
| `Alt+N` / `Alt+P` | Move to the next or previous match |
| `Alt+X` | Clear the query and highlights |

## Assignment 3 · Process memory snapshots

Added four snapshot slots per process and exposed them through xv6 system calls. Taking a snapshot copies the process's user address space and records its size. The comparison syscall counts pages whose contents differ or that are accessible in only one snapshot. The API also exposes snapshot metadata, finds a free slot, and releases saved memory.

Children receive independent copies of their parent's snapshots on `fork()`. The kernel frees partially copied snapshots if inheritance fails and releases remaining snapshots when a process exits. [Snapshot implementation](assignment-3/kernel/proc.c) · [System call handlers](assignment-3/kernel/sysproc.c) · [User-space demo](assignment-3/user/snaptest.c)

**Verified in QEMU.** Snapshot creation, slot error handling, status, page comparison, and release produced the expected results in the bundled demo. A separate check confirmed that a child can release an inherited snapshot while the parent's copy remains available.

**Known limitations.** The restore syscall attempts to replace the active user address space with a fresh copy of a snapshot. The bundled demo reboots the guest at the restore step in the tested configuration. The additional `snap_count_holes` syscall is incomplete and uses an uninitialized page-table entry pointer.

## Build and run

Use a Linux host with GNU Make, GCC and binutils with 32-bit x86 support, Perl, and QEMU for x86. Both builds were verified with GCC 15.2 and QEMU 10.2.

Run each example from the repository root. To try console search, launch the graphical QEMU target, type `echo Abc abc` at the xv6 prompt, then press `Alt+F` and type `abc`. This example and the search shortcuts were verified in QEMU.

```sh
make -C assignment-1
make -C assignment-1 qemu
```

For the snapshot demo, boot Assignment 3 in the terminal and enter `snaptest` at the xv6 prompt. The program prints its results up to the restore step described above.

```sh
make -C assignment-3
make -C assignment-3 qemu-nox
```

## Credits

Both assignments build on MIT's xv6 teaching kernel and retain its license notices. [Upstream xv6](https://github.com/mit-pdos/xv6-public) · [Assignment 1 license](assignment-1/LICENSE) · [Assignment 3 license](assignment-3/LICENSE)
