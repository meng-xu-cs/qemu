from pycpio.cpio import pad_cpio
from pycpio.header import CPIOHeader, HEADER_NEW

from pathlib import Path


class CPIOWriter:
    """
    Takes a list of CPIOData objects,
    writes them to the file specified by output_file.
    """

    def __init__(
        self, cpio_entries: list, output_file: Path, structure=None, *args, **kwargs
    ):
        self.cpio_entries = cpio_entries
        self.output_file = Path(output_file)

        self.structure = structure if structure is not None else HEADER_NEW

    def write(self):
        """
        Writes the CPIOData objects to the output file.
        """
        offset = 0
        with open(self.output_file, "wb") as f:
            for entry in self.cpio_entries.values():
                entry_bytes = bytes(entry)
                padding = pad_cpio(len(entry_bytes))
                output_bytes = entry_bytes + b"\x00" * padding

                f.write(output_bytes)
                offset += len(output_bytes)
            trailer = CPIOHeader(
                structure=self.structure,
                name="TRAILER!!!",
            )
            f.write(bytes(trailer))
            offset += len(bytes(trailer))
