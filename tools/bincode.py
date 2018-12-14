"""
bincode.py

IDA script that generates a C array of bytes with the disassembly in comments

"""
from __future__ import print_function
import os
import re
import sys
import operator

import idc
import idaapi
import idautils

if sys.version_info[0] == 2:
	try:
		from cStringIO import StringIO
	except:
		from StringIO import StringIO
else:
	from io import StringIO

# When an instruction's bytes contains the following, the bytes question will
# be zeroed out and a macro will be generated defined to their offset.
PLACEHOLDER_BYTES = (0xDE, 0xAD, 0xBE, 0xEF,)

# Templates for code generated

OUTPUT_PROLOGUE = """
static const uint8_t %s[] = {
""".strip() + '\n'

OUTPUT_EPILOGUE = """
};
""".strip() + '\n'

SIZE_TMPL = '#define {0}_SIZE sizeof({0})\n'
OFFSET_TMPL = '#define {}_OFFSET_{:d} 0x{:2X}\n'

# Internally used
RE_SPACES = re.compile(' {2,}')
PLACEHOLDER_SIZE = len(PLACEHOLDER_BYTES)
ZEROED_BYTES = (0x0,) * PLACEHOLDER_SIZE

def norm(text):
	return text.upper().strip() if text else ''

def xhex(byte):
	return '0x{:2X}'.format(byte)

def get_name(ea):
	name = idc.Name(ea)
	return name if name else None

def segms_by_class(cls):
	"""
	Get list of segments (sections) in the binary image, filtering based on
	the segment's class.

	@return: List of segment start addresses.
	"""
	cls = norm(cls)
	for n in xrange(idaapi.get_segm_qty()):
		seg = idaapi.getnseg(n)
		if seg and not seg.empty():
			segcls = norm(idaapi.get_segm_class(seg))
			if segcls == cls:
				yield seg

def insn_bytes(insn):
	size = insn.size
	start_ea = insn.ea
	next_ea = start_ea + size
	for ea in xrange(start_ea, next_ea):
		byte = Byte(ea)
		yield byte

def get_insn_bytes(insn):
	"""
	Gets a tuple of all the bytes underlying insn.
	"""
	return tuple(insn_bytes(insn))

def get_insn_disasm(insn):
	"""
	Wraps idc.GetDisasmEx with some cleaning of whitespace
	"""
	global RE_SPACES
	disasm = idc.GetDisasmEx(insn.ea, idc.GENDSM_FORCE_CODE)
	return RE_SPACES.sub(' ', disasm.replace('\t', ' '))

def extract_placeholder(insnex):
	global PLACEHOLDER_BYTES, PLACEHOLDER_SIZE, ZEROED_BYTES
	insn_size = insnex.size
	if insn_size <= PLACEHOLDER_SIZE:
		return None
	
	insn_bytes = insnex.bytes
	last_idx = insn_size - PLACEHOLDER_SIZE
	for idx in xrange(0, last_idx + 1):
		slice_end = idx+PLACEHOLDER_SIZE
		bytes = insn_bytes[ idx : slice_end ]
		if bytes == PLACEHOLDER_BYTES:
			insnex.bytes = insn_bytes[:idx] + ZEROED_BYTES + insn_bytes[slice_end:]
			return idx
	return None

class insn_ex:
	__slots__ = ('ea', 'insn', 'bytes', 'disasm', '_c_bytes', '_placeholder',)
	
	def __init__(self, ea, limit=None):
		"""
		Constructor
		"""
		assert(isLoaded(ea))
		self.ea = ea
		self._c_bytes = None
		self._placeholder = None
		has_limit = limit is not None
		self.insn = idautils.DecodeInstruction(ea)
		if self.insn is None or (has_limit and self.insn.size > limit):
			self.bytes = (Byte(ea),)
			self.disasm = 'db {:2X}h'.format(self.bytes[0])
		else:
			self.bytes = get_insn_bytes(self.insn)
			self.disasm = get_insn_disasm(self.insn)
			self._placeholder = extract_placeholder(self)
	
	def _get_c_bytes(self):
		return '\t' + ', '.join(map(xhex, self.bytes)) + ','
	
	@property
	def placeholder(self):
		if self._placeholder is None:
			return None
		return self.ea + self._placeholder
	
	@property
	def size(self):
		return 1 if self.insn is None else self.insn.size
	
	@property
	def next_ea(self):
		return self.ea + self.size
	
	@property
	def c_comment(self):
		return '//   {}'.format(self.disasm)
	
	@property
	def c_bytes(self):
		if self._c_bytes is None:
			self._c_bytes = self._get_c_bytes()
		return self._c_bytes
	
	@property
	def c_bytes_len(self):
		return len(self.c_bytes)
	
	def c_line(self, just):
		c_line = ''
		name = get_name(self.ea)
		if name: c_line += '\t'.ljust(just) + ('// {}:\n'.format(name))
		return c_line + self.c_bytes.ljust(just) + self.c_comment + '\n'

def segm_insn_exs(seg):
	end_ea = seg.endEA
	next_ea = seg.startEA
	while next_ea < end_ea:
		insn = insn_ex(next_ea, end_ea - next_ea)
		yield insn
		next_ea = insn.next_ea

def set_clipboard_text(text):
	from PySide import QtGui
	clippy = QtGui.QClipboard()
	clippy.setText(text)

def output_code(text):
	Message('\n===============================================\n')
	Message(text)
	Message('\n===============================================\n')

def main():
	global OUTPUT_PROLOGUE, OUTPUT_EPILOGUE, OFFSET_TMPL
	Message('\nbincode.py started.\n')
	Message('Prompting user for C identifier...\n> ')
	idc.SetStatus(idc.IDA_STATUS_WAITING)
	ident = AskStr('CODE_', 'Please enter the C identifier you\'d like used..')
	Message(ident + '\n')
	
	writer = StringIO()
	writer.write(OUTPUT_PROLOGUE % ident)
	lengetter = operator.attrgetter('c_bytes_len')
	
	placeholders = []
	Message('Generating code... ')
	idc.SetStatus(idc.IDA_STATUS_WORK)
	for seg in segms_by_class('CODE'):
		insns = tuple(segm_insn_exs(seg))
		bytes_just = max(map(lengetter, insns)) + 1
		for insn in insns:
			writer.write(insn.c_line(bytes_just))
			placeholder = insn.placeholder
			if placeholder is not None:
				placeholders.append(placeholder)
				Message('\nLocated a placeholder at offset: {:2X}h'.format(placeholder))
	
	writer.write(OUTPUT_EPILOGUE)
	if len(placeholders) > 0:
		writer.write('\n')
		Message('\nAdding placeholder offset macros..')
		for idx, offset in enumerate(placeholders):
			line = OFFSET_TMPL.format(ident, idx, offset)
			writer.write(line)
	
	text = writer.getvalue()
	
	Message('\nGeneration complete.')
	output_code(text)
	
	Message('\nCopying generated code to clipboard...')
	set_clipboard_text(text)
	Message('\nDone.\n')
	
	idc.SetStatus(idc.IDA_STATUS_READY)
	writer.close()

if __name__=='__main__':
	main()
