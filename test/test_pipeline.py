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
    source = Gst.ElementFactory.make("cef")
    sink = Gst.ElementFactory.make("autovideosink")
    source.set_property('javascript', 'alert("test0");')

    # Create the empty pipeline
    pipeline = Gst.Pipeline.new("test-pipeline")

    if not source or not sink or not pipeline:
        print("Not all elements could be created.")
        exit(-1)

    # Build the pipeline
    pipeline.add(source)
    pipeline.add(sink)
    if not Gst.Element.link(source, sink):
        print("Elements could not be linked.")
        exit(-1)

    # Start playing
    ret = pipeline.set_state(Gst.State.PLAYING)
    if ret == Gst.StateChangeReturn.FAILURE:
        print("Unable to set the pipeline to the playing state.")
        exit(-1)

    time.sleep(0.5)
    wrapper = """function __injectedOnload(){
                     alert('Hello');
                     document.getElementById('gsr').innerText ='lol';
                     window.removeEventListener('load', __injectedOnload);
                 }
                 window.addEventListener('load', __injectedOnload);"""
    # Modify the source's properties
    source.set_property('javascript', wrapper)
    source.set_property('javascript', "document.getElementById('gsr').innerText ='lol1'")
    time.sleep(2)
    source.set_property('javascript', 'alert("test3");')
    # time.sleep(2)
    # source.set_property('javascript', 'alert("test4");')
    # time.sleep(2)

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