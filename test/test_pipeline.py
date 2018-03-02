import sys
import gi
import time
gi.require_version('Gst', '1.0')
from gi.repository import Gst, GObject
Gst.init(None)

import threading


def run_main_loop():
    loop = GObject.MainLoop()
    loop.run()


def main():
    runner = threading.Thread(target=run_main_loop)
    runner.start()

    # Create the elements
    queue1 = Gst.ElementFactory.make('queue')
    queue2 = Gst.ElementFactory.make('queue')
    source = Gst.ElementFactory.make('cef')
    sink = Gst.ElementFactory.make('autovideosink')
    sink2 = Gst.ElementFactory.make('glimagesink')
    tee = Gst.ElementFactory.make('tee')

    # Create the empty pipeline
    pipeline = Gst.Pipeline.new("test-pipeline")

    if not source or not sink or not pipeline:
        print("Not all elements could be created.")
        exit(-1)

    # Build the pipeline
    pipeline.add(source)
    pipeline.add(sink)
    pipeline.add(tee)
    pipeline.add(sink2)
    pipeline.add(queue1)
    pipeline.add(queue2)

    Gst.Element.link(source, tee)
    Gst.Element.link(tee, queue1)
    Gst.Element.link(tee, queue2)
    Gst.Element.link(queue1, sink2)
    Gst.Element.link(queue2, sink)

    source.set_property('initialization-js', "document.getElementById('gsr').innerText ='lol1'")

    # Start playing
    ret = pipeline.set_state(Gst.State.PLAYING)
    if ret == Gst.StateChangeReturn.FAILURE:
        print("Unable to set the pipeline to the playing state.")
        exit(-1)

    time.sleep(2)
    for i in range(10):
        time.sleep(5)
        source.set_property('javascript', "document.getElementById('gsr').innerText ='{}'".format(i))
    pipeline.set_state(Gst.State.NULL)
    print('Sleeping for 20s')
    time.sleep(20)

    # Wait until error or EOS
    bus = pipeline.get_bus()
    msg = bus.timed_pop_filtered(
        Gst.CLOCK_TIME_NONE, Gst.MessageType.ERROR | Gst.MessageType.EOS)

    # Parse message
    if (msg):
        if msg.type == Gst.MessageType.ERROR:
            err, debug = msg.parse_error()
            print("Error received from element %s: %s" % (
                msg.src.get_name(), err))
            print("Debugging information: %s" % debug)
        elif msg.type == Gst.MessageType.EOS:
            print("End-Of-Stream reached.")
        else:
            print("Unexpected message received.")

    # Free resources
    pipeline.set_state(Gst.State.NULL)

if __name__ == '__main__':
    main()