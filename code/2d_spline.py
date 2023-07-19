import matplotlib.pyplot as plt
from math import factorial as fac
from math import pow
import math
import numpy as np
from functools import reduce
from matplotlib.backend_bases import MouseButton
import matplotlib as mpl

def main():
  steps=255
  # one dimensional bezier
  controls=[ [0,0], [1,2], [3,1], [3,-1], [3,-2], [4,-3], [5,-2], [5,0], [6,1], [7,1], [8,0], [8,-1], [9,-1], [10,0], [11,0],[12,-1], [11.5,-2.5], [10,-3], [9,-2], [10,0] ]
  doPlot(controls, point_from_bspline)

def lerp(a, b, t):
    return np.add(np.multiply(a, 1-t), np.multiply(b, t))

def bezier2(A, B, C, t):
    p0 = lerp(A, B, t)
    p1 = lerp(B, C, t)
    return lerp(p0, p1, t)

# fsplit(0, 4) -> (0, 0), fsplit(1, 4) -> (1, 3)
def fsplit(value, count):
  if(value < 0 or count == 0):
    return (0, 0)
  t = value * count
  int_num = math.floor(t)
  decimal = t - int_num
  if(value > 1 or int_num == count):
    return (1, count-1)
  return (decimal, int_num)

# use interpolated points as start and end
# and path points in between as control points
def point_from_bspline(points, t):
    bezier_number = len(points) - 2;

    t_sub, bezier_index = fsplit(t, bezier_number)

    pl = points[bezier_index]
    pm = points[bezier_index + 1]
    pr = points[bezier_index + 2]

    p_start = lerp(pl, pm, .5)
    p_end = lerp(pm, pr, .5)
    control = pm

    return bezier2(p_start, control, p_end, t_sub)

# curve_interpolater takes path/list of points and parameter t
def doPlot(controls, curve_interpolator, steps = 100):
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

    drawCurve([ curve_interpolator(controls, t) for i, t in enumerate(np.linspace(0, 1., steps))])
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
  fig = plt.figure("Simple simple editor")
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
