param([string]$Dll)
$ErrorActionPreference='Stop'
$b=[IO.File]::ReadAllBytes((Resolve-Path $Dll))
function U32([int]$p) { [BitConverter]::ToUInt32($b,$p) }
function U16([int]$p) { [BitConverter]::ToUInt16($b,$p) }
$pe=U32 0x3c
if((U32 $pe) -ne 0x4550) { throw 'Not a PE image' }
$opt=$pe+24
if((U16 $opt) -ne 0x20b) { throw 'Not PE32+' }
$sections=$opt+(U16 ($pe+20))
$count=U16 ($pe+6)
function Offset([uint32]$rva) {
    for($i=0;$i -lt $count;$i++) {
        $s=$sections+40*$i; $start=U32 ($s+12)
        $size=[Math]::Max((U32 ($s+8)),(U32 ($s+16)))
        if($rva -ge $start -and $rva -lt $start+$size) { return [int]((U32 ($s+20))+$rva-$start) }
    }
    throw "Unmapped RVA $rva"
}
function CString([int]$p) { $end=$p; while($b[$end] -ne 0) {$end++}; [Text.Encoding]::ASCII.GetString($b,$p,$end-$p) }
$exp=Offset (U32 ($opt+112))
$names=Offset (U32 ($exp+32)); $ords=Offset (U32 ($exp+36)); $funcs=Offset (U32 ($exp+28))
$data=-1
for($i=0;$i -lt (U32 ($exp+24));$i++) {
    if((CString (Offset (U32 ($names+4*$i)))) -eq 'SKSEPlugin_Version') {
        $data=Offset (U32 ($funcs+4*(U16 ($ords+2*$i)))); break
    }
}
if($data -lt 0) { throw 'Missing SKSEPlugin_Version export' }
$expected=@(0x01050610,0x01061610,0x01062800,0x010646a0,0x01064920,0x010649b1,0x01070680)
if((CString ($data+8)) -ne 'OutfitGallery') {throw 'Wrong plugin name'}
if((U32 ($data+4)) -ne 0x01000100) {throw 'Wrong plugin version'}
if((U32 ($data+0x308)) -ne 0) {throw 'Expected exact-list compatibility'}
for($i=0;$i -lt $expected.Count;$i++) {
    $actual=U32 ($data+0x30c+4*$i)
    if($actual -ne $expected[$i]) {throw "Compatibility entry $i mismatch: $actual"}
}
if((U32 ($data+0x30c+4*$expected.Count)) -ne 0) {throw 'Missing list terminator'}
'PASS: exported 1.0.16.0 metadata; six Steam versions retained; GOG=0x010649B1; exact list terminated.'
