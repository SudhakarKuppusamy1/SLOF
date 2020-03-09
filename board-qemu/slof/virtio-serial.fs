\ *****************************************************************************
\ * Copyright (c) 2016 IBM Corporation
\ * All rights reserved.
\ * This program and the accompanying materials
\ * are made available under the terms of the BSD License
\ * which accompanies this distribution, and is available at
\ * http://www.opensource.org/licenses/bsd-license.php
\ *
\ * Contributors:
\ *     IBM Corporation - initial implementation
\ ****************************************************************************/

s" serial" device-type

FALSE VALUE initialized?

virtio-setup-vd VALUE virtiodev

\ Quiescence the virtqueue of this device so that no more background
\ transactions can be pending.
: shutdown  ( -- )
    initialized? IF
        my-phandle node>path open-dev ?dup IF
            virtiodev virtio-serial-shutdown
            close-dev
        THEN
        FALSE to initialized?
    THEN
;

: virtio-serial-term-emit
    virtiodev SWAP virtio-serial-putchar
;

: virtio-serial-term-key?  virtiodev virtio-serial-haschar ;
: virtio-serial-term-key   BEGIN virtio-serial-term-key? UNTIL virtiodev virtio-serial-getchar ;

\ Basic device initialization - which has only to be done once
: init  ( -- )
virtiodev virtio-serial-init drop
    TRUE to initialized?
    ['] shutdown add-quiesce-xt
;

0 VALUE open-count

\ Standard node "open" function
: open  ( -- okay? )
    open-count 0= IF
        open IF initialized? 0= IF init THEN
            true
        ELSE false exit
        THEN
    ELSE true THEN
    open-count 1 + to open-count
;

: close
    open-count 0> IF
        open-count 1 - dup to open-count
        0= IF close THEN
    THEN
    close
;

: write ( addr len -- actual )
    tuck
    0 ?DO
        dup c@ virtiodev SWAP virtio-serial-putchar
        1 +
    LOOP
    drop
;

: read ( addr len -- actual )
    0= IF drop 0 EXIT THEN
    virtiodev virtio-serial-haschar 0= IF 0 swap c! -2 EXIT THEN
    virtiodev virtio-serial-getchar swap c! 1
;

: setup-alias
    " vsterm" find-alias 0= IF
        " vsterm" get-node node>path set-alias
    ELSE drop THEN
;
setup-alias

\ Override serial methods to make term-io.fs happy
: serial-emit virtio-serial-term-emit ;
: serial-key? virtio-serial-term-key? ;
: serial-key  virtio-serial-term-key  ;
