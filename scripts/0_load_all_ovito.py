import ovito
from ovito.data import *
from ovito.io import import_file

FILENAME_PATTERN="/tmp/foo/foo_{}.xyz"
NUM_CPUS=256

for i in ovito.dataset.scene_nodes:
    i.remove_from_scene()

for i in range(NUM_CPUS):
    node = import_file(FILENAME_PATTERN.format(i),
						columns = [None,"Position.X", "Position.Y"],
						multiple_frames = True )
	node.source.cell.display.render_cell = False
	node.add_to_scene()
