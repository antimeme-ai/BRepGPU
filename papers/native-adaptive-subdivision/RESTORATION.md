# Restore the retained publisher paper

The PDF is ignored; its original bytes are retained locally. To restore it:

```sh
mkdir -p papers/native-adaptive-subdivision
curl -fL -A 'Mozilla/5.0' \
  'https://diglib.eg.org/bitstreams/3faf6a3d-6dc3-454a-b654-b01cd65ab1f2/download' \
  -o papers/native-adaptive-subdivision/ma-hewitt-2002.pdf
printf '%s  %s\n' \
  5d4c32a367db55ba6baef163cbfa78d0f0621b4bb2df700ad36712292545d708 \
  papers/native-adaptive-subdivision/ma-hewitt-2002.pdf | shasum -a 256 -c -
```

The original download is kept intact. Text extraction is only a reading aid;
it is not a replacement archive. Existing NVIDIA and Balazs sources are restored
through `QUARANTINE.md` and the earlier whole-mesher acquisition records.
