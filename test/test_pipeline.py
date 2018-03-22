import os
import threading
import time

import gi
gi.require_version('Gst', '1.0')
from gi.repository import Gst, GObject
Gst.init(None)

loop = None


def run_main_loop(loop):
    loop.run()


def main():
    global loop
    print('The python.exe that this script is called with must be in the same directory as the cef resource files')
    loop = GObject.MainLoop()
    runner = threading.Thread(target=run_main_loop, args=(loop,))
    runner.start()

    # Create the elements
    queue1 = Gst.ElementFactory.make('queue')
    source = Gst.ElementFactory.make('cef')
    #source = Gst.ElementFactory.make('videotestsrc')
    sink = Gst.ElementFactory.make('glimagesink')
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
    pipeline.add(queue1)

    Gst.Element.link(source, tee)
    Gst.Element.link(tee, queue1)
    Gst.Element.link(queue1, sink)

    source.set_property('initialization-js', "document.getElementById('gsr').innerText ='0'")
    # Start playing
    ret = pipeline.set_state(Gst.State.PLAYING)
    if ret == Gst.StateChangeReturn.FAILURE:
        print("Unable to set the pipeline to the playing state.")
        exit(-1)

    time.sleep(2)
    for i in range(1, 5):
        time.sleep(2)
        source.set_property('javascript', "document.getElementById('gsr').innerText ='{}'".format(i))
    pipeline.set_state(Gst.State.NULL)
    print('Pipeline state set to null.  Sleeping for 10s')
    time.sleep(3)
    loop.quit()


def latency_query():
    global loop
    print('The python.exe that this script is called with must be in the same directory as the cef resource files')
    loop = GObject.MainLoop()
    runner = threading.Thread(target=run_main_loop, args=(loop,))
    runner.start()

    # Create the elements
    queue1 = Gst.ElementFactory.make('queue')
    #source = Gst.ElementFactory.make('cef')
    source = Gst.ElementFactory.make('videotestsrc')
    source.set_property('is-live', True)
    source.set_property('timestamp-offset', 100000000)
    sink = Gst.ElementFactory.make('glimagesink')
    tee = Gst.ElementFactory.make('tee')
    mixer = Gst.ElementFactory.make('glvideomixer')

    # Create the empty pipeline
    pipeline = Gst.Pipeline.new("test-pipeline")
    pipeline.set_latency(100000000)
    if not source or not sink or not pipeline:
        print("Not all elements could be created.")
        exit(-1)

    # Build the pipeline
    pipeline.add(source)
    pipeline.add(sink)
    pipeline.add(tee)
    pipeline.add(queue1)
    pipeline.add(mixer)

    Gst.Element.link(source, tee)
    Gst.Element.link(tee, queue1)
    Gst.Element.link(queue1, mixer)
    Gst.Element.link(mixer, sink)

    # Start playing
    ret = pipeline.set_state(Gst.State.PLAYING)
    if ret == Gst.StateChangeReturn.FAILURE:
        print("Unable to set the pipeline to the playing state.")
        exit(-1)

    time.sleep(2)
    for i in range(1, 5):
        time.sleep(2)
        print('Latency: {}'.format(pipeline.get_latency()))
    time.sleep(20)
    pipeline.set_state(Gst.State.NULL)
    print('Pipeline state set to null.  Sleeping for 10s')
    time.sleep(3)
    loop.quit()


def streamlabs_alert():
    global loop
    print('The python.exe that this script is called with must be in the same directory as the cef resource files')
    loop = GObject.MainLoop()
    runner = threading.Thread(target=run_main_loop, args=(loop,))
    runner.start()

    # Create the elements
    source = Gst.ElementFactory.make('cef')
    sink = Gst.ElementFactory.make('glimagesink')
    source.set_property('url', 'https://streamlabs.com/alert-box/v3/B32C8C284B4D610B2D79')
    # Create the empty pipeline
    pipeline = Gst.Pipeline.new("test-pipeline")

    if not source or not sink or not pipeline:
        print("Not all elements could be created.")
        exit(-1)

    # Build the pipeline
    pipeline.add(source)
    pipeline.add(sink)

    Gst.Element.link(source, sink)

    # Start playing
    ret = pipeline.set_state(Gst.State.PLAYING)
    if ret == Gst.StateChangeReturn.FAILURE:
        print("Unable to set the pipeline to the playing state.")
        exit(-1)

    time.sleep(20)
    pipeline.set_state(Gst.State.NULL)
    print('Pipeline state set to null.  Sleeping for 10s')
    time.sleep(3)
    loop.quit()


if __name__ == '__main__':
    try:
        # latency_query()
        # main()
        streamlabs_alert()
    except KeyboardInterrupt:
        loop.quit()