package main

import (
	"bytes"
	"encoding/binary"
	"flag"
	"fmt"
	"log"
	"os"
	"strings"
	"time"
)

const (
	MaximumEntrySize = 505
	MaximumBlockSize = 512
)

var (
	Crc16Table = []uint16{
		0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401,
		0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400,
	}
)

func Crc16Update(start uint16, data []byte, n int) uint16 {
	crc := start
	var r uint16

	for i := 0; i < n; i += 1 {
		b := data[i]
		/* compute checksum of lower four bits of *p */
		r = Crc16Table[crc&0xF]
		crc = (crc >> 4) & 0x0FFF
		crc = crc ^ r ^ Crc16Table[b&0xF]
		/* now compute checksum of upper four bits of *p */
		r = Crc16Table[crc&0xF]
		crc = (crc >> 4) & 0x0FFF
		crc = crc ^ r ^ Crc16Table[(b>>4)&0xF]
	}

	return crc
}

type Entry struct {
	File      uint8
	Size      uint16
	Available uint16
	Crc       uint16
}

type File struct {
	Name       [12]byte
	Version    uint16
	StartBlock uint32
	EndBlock   uint32
	Size       uint32
}

type HeaderBlock struct {
	Version    uint8
	Generation uint32
	Block      uint32
	Offset     uint16
	Time       uint32
	Files      [6]File
	Crc        uint16
}

func ReadHeader(f *os.File) *HeaderBlock {
	headerBlock := [2]HeaderBlock{}
	err := binary.Read(f, binary.LittleEndian, &headerBlock)
	if err != nil {
		panic(err)
	}

	if headerBlock[0].Generation > headerBlock[1].Generation {
		return &headerBlock[0]
	}

	return &headerBlock[1]
}

type Block struct {
	File  *File
	Entry *Entry
	Data  []byte
	Next  Cursor
}

type Cursor struct {
	Block  uint32
	Offset uint16
}

func BlockChecksum(file *File, entry *Entry, data []byte) uint16 {
	crc := uint16(file.Version)

	b := new(bytes.Buffer)
	binary.Write(b, binary.LittleEndian, entry)

	crc = Crc16Update(crc, b.Bytes(), len(b.Bytes())-2)
	crc = Crc16Update(crc, data, len(data))

	return crc
}

func ReadBlock(header *HeaderBlock, c Cursor, f *os.File) *Block {
	position := int64(c.Block*512 + uint32(c.Offset))

	f.Seek(position, 0)

	entry := Entry{}
	err := binary.Read(f, binary.LittleEndian, &entry)
	if err != nil {
		panic(err)
	}

	if entry.File >= 6 || entry.Size == 0 || entry.Size > MaximumEntrySize || entry.Available == 0 || entry.Available > MaximumEntrySize {
		return &Block{
			Next: Cursor{
				Block:  c.Block + 1,
				Offset: 0,
			},
		}
	}

	data := make([]byte, entry.Size)

	_, err = f.Read(data)
	if err != nil {
		panic(err)
	}

	actual := BlockChecksum(&header.Files[entry.File], &entry, data)

	if entry.Crc != actual {
		return &Block{
			Next: Cursor{
				Block:  c.Block + 1,
				Offset: 0,
			},
		}
	}

	next := Cursor{
		Block:  c.Block,
		Offset: c.Offset + entry.Size + uint16(binary.Size(&entry)),
	}
	if next.Offset >= MaximumBlockSize {
		next.Block = c.Block + 1
		next.Offset = 0
	}

	return &Block{
		File:  &header.Files[entry.File],
		Entry: &entry,
		Data:  data,
		Next:  next,
	}
}

type options struct {
	Card string
}

type FileInfo struct {
	Name string
	File *os.File
	Size int
}

func main() {
	o := options{}

	flag.StringVar(&o.Card, "card", "", "card/image to read")

	flag.Parse()

	if o.Card == "" {
		flag.Usage()
		os.Exit(2)
	}

	f, err := os.Open(o.Card)
	if err != nil {
		panic(err)
	}

	defer f.Close()

	header := ReadHeader(f)

	c := Cursor{
		Block:  0,
		Offset: 0,
	}

	files := make(map[uint8]*FileInfo)

	prefix := time.Now().Format("20060102_150405")

	for c.Block = 1; c.Block < header.Block; {
		if c.Block == 0 {
			c.Block += 1
			continue
		}

		b := ReadBlock(header, c, f)

		if b.Entry != nil {
			if files[b.Entry.File] == nil {
				name := fmt.Sprintf("%s_%s", prefix, strings.TrimRight(string(b.File.Name[:]), "\x00"))

				log.Printf("Exporting %s...", name)

				wf, err := os.OpenFile(name, os.O_RDWR|os.O_CREATE|os.O_TRUNC, 0644)
				if err != nil {
					panic(err)
				}

				defer wf.Close()

				files[b.Entry.File] = &FileInfo{
					Name: name,
					File: wf,
					Size: 0,
				}
			}

			_, err = files[b.Entry.File].File.Write(b.Data)
			if err != nil {
				panic(err)
			}

			files[b.Entry.File].Size += len(b.Data)

		}

		c = b.Next
	}

	for _, file := range files {
		log.Printf("Saved %s (%d bytes)", file.Name, file.Size)
	}
}
