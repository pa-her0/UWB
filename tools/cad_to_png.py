#!/usr/bin/env python3
"""
cad_to_png.py  <input.dxf>  <output.png>

Converts a DXF file to a PNG image suitable for use as a floor plan
background in the UWB RTLS positioning system.

Requirements:
    pip install ezdxf matplotlib
"""

import sys
import os

def convert(dxf_path, png_path):
    try:
        import ezdxf
    except ImportError:
        print("ERROR: ezdxf not installed. Run: pip install ezdxf", file=sys.stderr)
        return 1

    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
        from matplotlib.patches import Arc, Circle
        from matplotlib.lines import Line2D
    except ImportError:
        print("ERROR: matplotlib not installed. Run: pip install matplotlib", file=sys.stderr)
        return 1

    try:
        doc = ezdxf.readfile(dxf_path)
    except Exception as e:
        print(f"ERROR: Cannot read DXF file: {e}", file=sys.stderr)
        return 1

    msp = doc.modelspace()

    fig, ax = plt.subplots(figsize=(16, 12), dpi=150)
    ax.set_aspect("equal")
    ax.axis("off")
    fig.patch.set_facecolor("white")
    ax.set_facecolor("white")

    line_kw = dict(color="black", linewidth=0.5, solid_capstyle="round")

    for entity in msp:
        dxftype = entity.dxftype()
        try:
            if dxftype == "LINE":
                s, e = entity.dxf.start, entity.dxf.end
                ax.plot([s.x, e.x], [s.y, e.y], **line_kw)

            elif dxftype == "LWPOLYLINE":
                pts = list(entity.get_points("xy"))
                if not pts:
                    continue
                xs = [p[0] for p in pts]
                ys = [p[1] for p in pts]
                if entity.closed:
                    xs.append(xs[0])
                    ys.append(ys[0])
                ax.plot(xs, ys, **line_kw)

            elif dxftype == "POLYLINE":
                pts = [v.dxf.location for v in entity.vertices]
                if not pts:
                    continue
                xs = [p.x for p in pts]
                ys = [p.y for p in pts]
                if entity.is_closed:
                    xs.append(xs[0])
                    ys.append(ys[0])
                ax.plot(xs, ys, **line_kw)

            elif dxftype == "CIRCLE":
                c = entity.dxf.center
                r = entity.dxf.radius
                circle = plt.Circle((c.x, c.y), r, fill=False, **line_kw)
                ax.add_patch(circle)

            elif dxftype == "ARC":
                c = entity.dxf.center
                r = entity.dxf.radius
                a1 = entity.dxf.start_angle
                a2 = entity.dxf.end_angle
                arc = Arc((c.x, c.y), 2*r, 2*r, angle=0,
                          theta1=a1, theta2=a2, **line_kw)
                ax.add_patch(arc)

            elif dxftype in ("TEXT", "MTEXT"):
                try:
                    if dxftype == "TEXT":
                        pos = entity.dxf.insert
                        txt = entity.dxf.text
                        h = entity.dxf.get("height", 0.25)
                    else:
                        pos = entity.dxf.insert
                        txt = entity.plain_mtext()
                        h = entity.dxf.get("char_height", 0.25)
                    ax.text(pos.x, pos.y, txt, fontsize=max(4, h * 8),
                            color="black", va="bottom")
                except Exception:
                    pass

            elif dxftype == "SPLINE":
                try:
                    pts = list(entity.flattening(0.01))
                    if pts:
                        xs = [p[0] for p in pts]
                        ys = [p[1] for p in pts]
                        ax.plot(xs, ys, **line_kw)
                except Exception:
                    pass

        except Exception:
            pass  # skip unsupported entities silently

    ax.autoscale_view()
    plt.tight_layout(pad=0)

    try:
        fig.savefig(png_path, dpi=150, bbox_inches="tight",
                    facecolor="white", format="png")
        plt.close(fig)
    except Exception as e:
        print(f"ERROR: Cannot save PNG: {e}", file=sys.stderr)
        return 1

    print(f"OK: {png_path}")
    return 0


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <input.dxf> <output.png>", file=sys.stderr)
        sys.exit(1)
    sys.exit(convert(sys.argv[1], sys.argv[2]))
