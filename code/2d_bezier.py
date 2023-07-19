import matplotlib.pyplot as plt
from math import factorial as fac
from math import pow
import numpy as np
from functools import reduce
from matplotlib.backend_bases import MouseButton
import matplotlib as mpl

def main():
  steps=255

  controls=[[ 0, 0.],
          [ 0, .7],
          [-.1, 1],
          [ .2, 1.]]
  doPlot(controls)

def lerp(a, b, t):
    return np.add(np.multiply(a, 1-t), np.multiply(b, t))

def bezier3deg(A, B, C, D, t):
  p0 = lerp(A, B, t)
  p1 = lerp(B, C, t)
  p2 = lerp(C, D, t)

  p01 = lerp(p0, p1, t)
  p12 = lerp(p1, p2, t)

  p0112 = lerp(p01, p12, t)
  return p0112

def doPlot(controls, steps = 100):
  def drawControls(points):
    for i, p in enumerate(points):
      circe_size = mpl.rcParams['lines.markersize'] ** 2
      plt.scatter(*p, s = circe_size * 2.5 if i == dragIndex else circe_size)
    plt.plot(*zip(*points), '--', linewidth=0.5)

  def drawCurve(path):
    xs, ys = zip(*path)
    plt.plot(xs, ys)

  def updatePlot():
    for artist in plt.gca().lines + plt.gca().collections:
      artist.remove()

    drawCurve([ bezier3deg(*controls, t) for i, t in enumerate(np.linspace(0, 1., steps))])
    drawControls(controls)
    fig.canvas.draw()

  def fromDataToScreen(p):
    return ax.transData.transform(p)

  def norm(p0, p1):
    return np.linalg.norm(np.subtract(p1, p0))

  def getClosestControl(click_point):
    n, i = min([ [norm(click_point, fromDataToScreen(p)), i] for i, p in enumerate(controls) ])
    return i, n
  def getClickedControl(click_point):
    i, n = getClosestControl(click_point)
    return i if n <= 5 else -1

  def on_click(event):
    if not event.inaxes:
      return
    if event.button is MouseButton.LEFT:
      on_left_click(event)
    elif event.button is MouseButton.RIGHT:
      on_right_click(event)
    elif event.dblclick:
      pass

  def on_left_click(event):
    global dragIndex
    click_pos = [event.x, event.y]
    dragIndex = getClickedControl(click_pos)
    updatePlot() # createNewThing (create between?)

  def on_right_click(event):
    global dragIndex
    if dragIndex >= 0:
      controls[dragIndex] = [event.xdata, event.ydata]
      updatePlot()

  def on_key(event):
    #print('press', event.key)
    if event.key == ' ' or event.key == 'enter':
      print(controls)

  global dragIndex
  dragIndex = -1

  # set up plot
  fig = plt.figure("3deg bezier editor")
  limits = plt.axis('equal')
  ax = plt.gca()
  plt.xlabel('X')
  plt.ylabel('Y')
  plt.grid(True)

  # plt.connect(..)
  cid = fig.canvas.mpl_connect('button_press_event', on_click)
  kid = fig.canvas.mpl_connect('key_press_event', on_key)

  # draw
  updatePlot()
  plt.show()


if __name__ == "__main__":
  main()
