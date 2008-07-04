#!/bin/sh

exiftoolDir="$EXIFTOOL_DIR"
resdir="$TARGET_BUILD_DIR/Hugin.app/Contents/Resources"

rm -rf $resdir/ExifTool;
mkdir -p $resdir/ExifTool;

cp $exiftoolDir/exiftool $resdir/ExifTool/;
cp -r $exiftoolDir/lib $resdir/ExifTool/;