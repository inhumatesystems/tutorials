using System;
using System.Collections.Generic;
using System.Linq;
using Godot;
using Inhumate.RTI;
using Inhumate.RTI.Proto;

namespace Inhumate.GodotRTI {

    public enum GeometryShape {
        Auto,
        Point,
        Point3D,
        Line,
        Line3D,
        Polygon,
        Mesh,
        Spline,
        Spline3D
    }

    public enum GeometryCoordinates {
        LocalAndGeodetic,
        Local,
        Geodetic
    }

    [GlobalClass, Icon("res://addons/inhumate_rti/icons/rti_geometry.svg")]
    public partial class RTIGeometry : Node {

        // Set to Auto to detect from sibling nodes (MeshInstance3D, Path3D, CollisionShape3D, CsgShape3D, Line2D).
        [Export] public GeometryShape Shape = GeometryShape.Auto;
        // Which coordinate systems to include when publishing geometry points.
        [Export] public GeometryCoordinates Coordinates = GeometryCoordinates.LocalAndGeodetic;
        // A string to identify this geometry type in other applications. OK to leave blank.
        [Export] public string Type = "";
        // Category can be used to distinguish or filter geometry in a scenario viewer.
        [Export] public Geometry.Types.Category Category = Geometry.Types.Category.UnknownGeometry;
        // Color used to represent this geometry. Not used if fully transparent (default).
        [Export] public Godot.Color Color = new Godot.Color(0, 0, 0, 0);
        // From 0 (transparent) to 1 (opaque).
        [Export(PropertyHint.Range, "0,1")] public float Opacity = 1f;
        // Title to show in the label. Leave blank to not show a label.
        [Export] public string Title = "";
        // If checked, the parent node's name is used as the geometry title.
        [Export] public bool TitleFromName;
        // Color used for the label. Not used if fully transparent (default).
        [Export] public Godot.Color LabelColor = new Godot.Color(0, 0, 0, 0);
        // From 0 (transparent) to 1 (opaque).
        [Export(PropertyHint.Range, "0,1")] public float LabelOpacity = 1f;
        // If checked, render in wireframe instead of solid.
        [Export] public bool Wireframe;
        // Width of lines for wireframe or line/spline geometry.
        [Export] public float LineWidth = 1f;
        // Unique ID for this geometry instance. Leave blank to auto-generate.
        [Export] public string Id = "";

        public bool Persistent { get; internal set; } = true;
        public bool Published { get; internal set; }
        public bool Owned { get; internal set; } = true;

        protected static RTIConnection RTI => RTIConnection.Instance;

        private bool useLocalCoordinates => Coordinates != GeometryCoordinates.Geodetic;
        private bool useGeodeticCoordinates => Coordinates != GeometryCoordinates.Local;

        private bool updateRequested;
        private bool registered;

        public void RequestUpdate() {
            updateRequested = true;
        }

        public override void _EnterTree() {
            if (RTI.State == RuntimeState.Loading || (RTI.Time < float.Epsilon && RTI.State != RuntimeState.Running)) {
                Persistent = true;
            }
            if (string.IsNullOrWhiteSpace(Id)) Id = GenerateId();
        }

        public override void _Ready() {
            if (RTI.GetGeometryById(Id) != null) {
                GD.PrintErr($"Geometry {Id} has already been registered");
            }
            registered = RTI.RegisterGeometry(this);
        }

        string GenerateId() {
            if (Persistent) {
                string path = "";
                var current = GetParent();
                while (current != null) {
                    path = "/" + current.Name.ToString().Trim().Replace(" (", "").Replace(")", "").Replace(" ", "_") + path;
                    current = current.GetParent();
                }
                return RTI.Application + path;
            } else {
                return Guid.NewGuid().ToString();
            }
        }

        public override void _Process(double deltaTime) {
            if (registered && Owned && RTI.Connected) {
                if (!Published) {
                    if (Persistent && RTI.persistentGeometryOwnerClientId != null && !RTI.IsPersistentGeometryOwner) {
                        Owned = false;
                    } else {
                        if (RTI.DebugEntities) GD.Print($"RTI publish create geometry {Id}");
                        RTI.Publish(RTIChannel.Geometry, GeometryData);
                    }
                    Published = true;
                } else if (updateRequested) {
                    updateRequested = false;
                    Publish();
                }
            }
        }

        void Publish() {
            RTI.Publish(RTIChannel.Geometry, GeometryData);
        }

        public override void _Notification(int what) {
            if (what == NotificationWMCloseRequest) {
                if (Published && Owned && (!Persistent || RTI.Quitting)) _ExitTree();
            }
        }

        public override void _ExitTree() {
            bool hopefullyOtherClientTakesOver = RTI.Quitting && Persistent && RTI.Client.KnownClients.Count(c => c.Application == RTI.Client.Application) > 1;
            if (registered && Published && Owned && RTI.Connected && !hopefullyOtherClientTakesOver) {
                Published = false;
                if (RTI.DebugEntities) GD.Print($"RTI publish deleted geometry {Id}");
                RTI.Publish(RTIChannel.Geometry, new Geometry {
                    Id = Id,
                    OwnerClientId = RTI.ClientId,
                    Deleted = true
                });
            }
            RTI.UnregisterGeometry(this);
        }

        // --- Geometry data construction ---

        internal Geometry GeometryData {
            get {
                var data = new Geometry {
                    Id = Id,
                    OwnerClientId = RTI.ClientId,
                    Type = Type,
                    Category = Category,
                    Transparency = 1f - Opacity,
                    LabelTransparency = 1f - LabelOpacity,
                    Wireframe = Wireframe,
                    LineWidth = LineWidth,
                    Title = TitleFromName ? GetParent().Name.ToString() : !string.IsNullOrWhiteSpace(Title) ? Title : "",
                };
                var col = GetProtoColor(Color);
                if (col != null) data.Color = col;
                var labelCol = GetProtoColor(LabelColor);
                if (labelCol != null) data.LabelColor = labelCol;
                ApplyShape(data);
                return data;
            }
        }

        void ApplyShape(Geometry data) {
            var shape = Shape;
            if (shape == GeometryShape.Auto) shape = DetectShape();

            var parent = GetParentOrNull<Node3D>();
            switch (shape) {
                case GeometryShape.Point:
                    if (parent != null) data.Point = CreatePoint2D(parent.GlobalPosition);
                    break;
                case GeometryShape.Point3D:
                    if (parent != null) data.Point3D = CreatePoint3D(parent.GlobalPosition);
                    break;
                case GeometryShape.Line:
                    data.Line = DetectLine2D();
                    break;
                case GeometryShape.Line3D:
                    data.Line3D = DetectLine3D();
                    break;
                case GeometryShape.Spline:
                    data.Spline = DetectSpline2D();
                    break;
                case GeometryShape.Spline3D:
                    data.Spline3D = DetectSpline3D();
                    break;
                case GeometryShape.Polygon:
                    data.Polygon = DetectPolygon();
                    break;
                case GeometryShape.Mesh:
                    data.Mesh = DetectMesh();
                    break;
            }
        }

        // --- Auto-detection ---

        GeometryShape DetectShape() {
            var parent = GetParent();

            // Path3D → Spline3D (has bezier tangent data)
            var path3D = FindNodeInContext<Path3D>(parent);
            if (path3D?.Curve != null && path3D.Curve.PointCount > 0) {
                return GeometryShape.Spline3D;
            }

            // MeshInstance3D → Mesh
            var meshInstance = FindNodeInContext<MeshInstance3D>(parent);
            if (meshInstance?.Mesh != null) {
                return GeometryShape.Mesh;
            }

            // CsgShape3D → Polygon (skip CsgCombiner3D)
            if (FindCsgPrimitives(parent).Count > 0) {
                return GeometryShape.Polygon;
            }

            // CollisionShape3D → Polygon (from shape bounds)
            var collisionShapes = FindNodesInContext<CollisionShape3D>(parent);
            if (collisionShapes.Count > 0) {
                return GeometryShape.Polygon;
            }

            // Line2D → Line
            var line2D = FindNodeInContext<Line2D>(parent);
            if (line2D != null) {
                return GeometryShape.Line;
            }

            // Fallback to point
            if (parent is Node3D) return GeometryShape.Point3D;
            return GeometryShape.Point;
        }

        // --- Shape builders ---

        Geometry.Types.Line2D DetectLine2D() {
            var parent = GetParent();
            var path3D = FindNodeInContext<Path3D>(parent);
            if (path3D?.Curve != null) return CreateLine2DFromPath3D(path3D);
            var line2D = FindNodeInContext<Line2D>(parent);
            if (line2D != null) return CreateLine2DFromLine2D(line2D);
            return new Geometry.Types.Line2D();
        }

        Geometry.Types.Line3D DetectLine3D() {
            var parent = GetParent();
            var path3D = FindNodeInContext<Path3D>(parent);
            if (path3D?.Curve != null) return CreateLine3DFromPath3D(path3D);
            return new Geometry.Types.Line3D();
        }

        Geometry.Types.Spline2D DetectSpline2D() {
            var parent = GetParent();
            var path3D = FindNodeInContext<Path3D>(parent);
            if (path3D?.Curve != null) return CreateSpline2DFromPath3D(path3D);
            return new Geometry.Types.Spline2D();
        }

        Geometry.Types.Spline3D DetectSpline3D() {
            var parent = GetParent();
            var path3D = FindNodeInContext<Path3D>(parent);
            if (path3D?.Curve != null) return CreateSpline3DFromPath3D(path3D);
            return new Geometry.Types.Spline3D();
        }

        Geometry.Types.Polygon DetectPolygon() {
            var parent = GetParent();

            var collisionShapes = FindNodesInContext<CollisionShape3D>(parent);
            if (collisionShapes.Count > 0) return CreatePolygonFromCollisionShapes(collisionShapes);

            var csgs = FindCsgPrimitives(parent);
            if (csgs.Count > 0) return CreatePolygonFromCsgPrimitives(csgs);

            var meshInstance = FindNodeInContext<MeshInstance3D>(parent);

            if (meshInstance?.Mesh != null) return CreatePolygonFromMeshInstance(meshInstance);

            return new Geometry.Types.Polygon();
        }

        Geometry.Types.Mesh DetectMesh() {
            var parent = GetParent();

            var meshInstances = FindNodesInContext<MeshInstance3D>(parent);
            if (meshInstances.Count > 0) return CreateMeshFromMeshInstances(meshInstances);

            var collisionShapes = FindNodesInContext<CollisionShape3D>(parent);
            if (collisionShapes.Count > 0) return CreateMeshFromCollisionShapes(collisionShapes);

            var csgs = FindCsgPrimitives(parent);
            if (csgs.Count > 0) return CreateMeshFromCsgPrimitives(csgs);

            return new Geometry.Types.Mesh();
        }

        // --- Node discovery helpers ---

        static T FindNodeInContext<T>(Node context) where T : Node {
            if (context is T t) return t;
            return context?.FindChildOfType<T>();
        }

        static List<T> FindNodesInContext<T>(Node context) where T : Node {
            var results = new List<T>();
            if (context is T t) results.Add(t);
            if (context != null) FindNodesRecursive(context, results);
            return results;
        }

        static void FindNodesRecursive<T>(Node parent, List<T> results) where T : Node {
            foreach (Node child in parent.GetChildren()) {
                if (child is T t) results.Add(t);
                FindNodesRecursive(child, results);
            }
        }

        /// <summary>
        /// Find all CSG primitive shapes, skipping CsgCombiner3D nodes whose AABB
        /// is just the bounding box of all children and not useful geometry.
        /// </summary>
        static List<CsgShape3D> FindCsgPrimitives(Node context) {
            var results = new List<CsgShape3D>();
            if (context != null) FindCsgPrimitivesRecursive(context, results);
            return results;
        }

        static void FindCsgPrimitivesRecursive(Node node, List<CsgShape3D> results) {
            if (node is CsgShape3D csg && csg is not CsgCombiner3D) {
                results.Add(csg);
                return;
            }
            foreach (Node child in node.GetChildren()) {
                FindCsgPrimitivesRecursive(child, results);
            }
        }

        // --- Path3D / Curve3D conversion ---

        Geometry.Types.Line2D CreateLine2DFromPath3D(Path3D path) {
            var line = new Geometry.Types.Line2D();
            var curve = path.Curve;
            for (int i = 0; i < curve.PointCount; i++) {
                var worldPos = path.GlobalTransform * curve.GetPointPosition(i);
                line.Points.Add(CreatePoint2D(worldPos));
            }
            return line;
        }

        Geometry.Types.Line3D CreateLine3DFromPath3D(Path3D path) {
            var line = new Geometry.Types.Line3D();
            var curve = path.Curve;
            for (int i = 0; i < curve.PointCount; i++) {
                var worldPos = path.GlobalTransform * curve.GetPointPosition(i);
                line.Points.Add(CreatePoint3D(worldPos));
            }
            return line;
        }

        Geometry.Types.Spline2D CreateSpline2DFromPath3D(Path3D path) {
            var spline = new Geometry.Types.Spline2D();
            var curve = path.Curve;
            for (int i = 0; i < curve.PointCount; i++) {
                var pos = path.GlobalTransform * curve.GetPointPosition(i);
                var tangentOut = path.GlobalTransform.Basis * curve.GetPointOut(i);
                spline.Points.Add(CreatePoint2D(pos));
                spline.ControlPoints.Add(CreatePoint2D(pos + tangentOut));
            }
            return spline;
        }

        Geometry.Types.Spline3D CreateSpline3DFromPath3D(Path3D path) {
            var spline = new Geometry.Types.Spline3D();
            var curve = path.Curve;
            for (int i = 0; i < curve.PointCount; i++) {
                var pos = path.GlobalTransform * curve.GetPointPosition(i);
                var tangentOut = path.GlobalTransform.Basis * curve.GetPointOut(i);
                spline.Points.Add(CreatePoint3D(pos));
                spline.ControlPoints.Add(CreatePoint3D(pos + tangentOut));
            }
            return spline;
        }

        // --- Line2D (Godot 2D) conversion ---

        Geometry.Types.Line2D CreateLine2DFromLine2D(Line2D line2D) {
            var line = new Geometry.Types.Line2D();
            foreach (var point in line2D.Points) {
                var worldPos = line2D.GlobalTransform * new Vector2(point.X, point.Y);
                line.Points.Add(CreatePoint2DFrom2D(worldPos));
            }
            return line;
        }

        // --- MeshInstance3D conversion ---

        Geometry.Types.Mesh CreateMeshFromMeshInstances(List<MeshInstance3D> meshInstances) {
            var outMesh = new Geometry.Types.Mesh();
            int offset = 0;
            foreach (var meshInstance in meshInstances) {
                var mesh = meshInstance.Mesh;
                if (mesh == null) continue;
                for (int s = 0; s < mesh.GetSurfaceCount(); s++) {
                    var arrays = mesh.SurfaceGetArrays(s);
                    if (arrays == null || arrays.Count == 0) continue;

                    var vertices = arrays[(int)Godot.Mesh.ArrayType.Vertex].AsVector3Array();
                    var indices = arrays[(int)Godot.Mesh.ArrayType.Index].AsInt32Array();

                    if (vertices != null) {
                        foreach (var vertex in vertices) {
                            var worldPos = meshInstance.GlobalTransform * vertex;
                            outMesh.Vertices.Add(ToLocalPoint3D(worldPos));
                        }
                    }
                    if (indices != null && indices.Length > 0) {
                        foreach (var index in indices) {
                            outMesh.Indices.Add(offset + index);
                        }
                    } else if (vertices != null) {
                        for (int i = 0; i < vertices.Length; i++) {
                            outMesh.Indices.Add(offset + i);
                        }
                    }
                    offset += vertices?.Length ?? 0;
                }
            }
            return outMesh;
        }

        // --- CollisionShape3D conversion ---

        Geometry.Types.Polygon CreatePolygonFromCollisionShapes(List<CollisionShape3D> shapes) {
            var bounds = new Aabb();
            bool first = true;
            foreach (var shape in shapes) {
                var shapeAabb = GetCollisionShapeAabb(shape);
                if (first) {
                    bounds = shapeAabb;
                    first = false;
                } else {
                    bounds = bounds.Merge(shapeAabb);
                }
            }
            return CreatePolygonFromWorldAabb(bounds);
        }

        Geometry.Types.Mesh CreateMeshFromCollisionShapes(List<CollisionShape3D> shapes) {
            var outMesh = new Geometry.Types.Mesh();
            int offset = 0;
            foreach (var shape in shapes) {
                if (shape.Shape is BoxShape3D box) {
                    var halfExtents = box.Size / 2f;
                    foreach (var corner in BOX_CORNERS) {
                        var localPos = new Vector3(
                            corner.X * box.Size.X - halfExtents.X,
                            corner.Y * box.Size.Y - halfExtents.Y,
                            corner.Z * box.Size.Z - halfExtents.Z
                        );
                        var worldPos = shape.GlobalTransform * localPos;
                        outMesh.Vertices.Add(ToLocalPoint3D(worldPos));
                    }
                } else {
                    var aabb = GetCollisionShapeAabb(shape);
                    foreach (var corner in BOX_CORNERS) {
                        var pos = new Vector3(
                            aabb.Position.X + corner.X * aabb.Size.X,
                            aabb.Position.Y + corner.Y * aabb.Size.Y,
                            aabb.Position.Z + corner.Z * aabb.Size.Z
                        );
                        outMesh.Vertices.Add(ToLocalPoint3D(pos));
                    }
                }
                foreach (var index in BOX_INDICES) {
                    outMesh.Indices.Add(offset + index);
                }
                offset += BOX_CORNERS.Length;
            }
            return outMesh;
        }

        Aabb GetCollisionShapeAabb(CollisionShape3D shape) {
            if (shape.Shape == null) return new Aabb();
            var localAabb = shape.Shape.GetDebugMesh().GetAabb();
            var worldMin = shape.GlobalTransform * localAabb.Position;
            var worldMax = shape.GlobalTransform * localAabb.End;
            var min = new Vector3(
                Mathf.Min(worldMin.X, worldMax.X),
                Mathf.Min(worldMin.Y, worldMax.Y),
                Mathf.Min(worldMin.Z, worldMax.Z)
            );
            var max = new Vector3(
                Mathf.Max(worldMin.X, worldMax.X),
                Mathf.Max(worldMin.Y, worldMax.Y),
                Mathf.Max(worldMin.Z, worldMax.Z)
            );
            return new Aabb(min, max - min);
        }

        // --- CSG conversion (using shape properties directly, like CollisionShape3D) ---

        // Get local AABB from CSG primitive properties, independent of combiner state.
        static Aabb GetCsgLocalAabb(CsgShape3D csg) {
            if (csg is CsgBox3D box) {
                var half = box.Size / 2f;
                return new Aabb(-half, box.Size);
            } else if (csg is CsgCylinder3D cyl) {
                var half = new Vector3(cyl.Radius, cyl.Height / 2f, cyl.Radius);
                return new Aabb(-half, half * 2f);
            } else if (csg is CsgSphere3D sphere) {
                var half = new Vector3(sphere.Radius, sphere.Radius, sphere.Radius);
                return new Aabb(-half, half * 2f);
            } else if (csg is CsgTorus3D torus) {
                var outer = torus.OuterRadius;
                var half = new Vector3(outer, torus.InnerRadius, outer);
                return new Aabb(-half, half * 2f);
            }
            // Fallback for CsgPolygon3D, CsgMesh3D, etc.
            return csg.GetAabb();
        }

        Geometry.Types.Polygon CreatePolygonFromCsgPrimitives(List<CsgShape3D> csgs) {
            var bounds = new Aabb();
            bool first = true;
            foreach (var csg in csgs) {
                var localAabb = GetCsgLocalAabb(csg);
                var worldMin = csg.GlobalTransform * localAabb.Position;
                var worldMax = csg.GlobalTransform * localAabb.End;
                var min = new Vector3(Mathf.Min(worldMin.X, worldMax.X), Mathf.Min(worldMin.Y, worldMax.Y), Mathf.Min(worldMin.Z, worldMax.Z));
                var max = new Vector3(Mathf.Max(worldMin.X, worldMax.X), Mathf.Max(worldMin.Y, worldMax.Y), Mathf.Max(worldMin.Z, worldMax.Z));
                var worldAabb = new Aabb(min, max - min);
                if (first) {
                    bounds = worldAabb;
                    first = false;
                } else {
                    bounds = bounds.Merge(worldAabb);
                }
            }
            return CreatePolygonFromWorldAabb(bounds);
        }

        Geometry.Types.Mesh CreateMeshFromCsgPrimitives(List<CsgShape3D> csgs) {
            var outMesh = new Geometry.Types.Mesh();
            int offset = 0;
            foreach (var csg in csgs) {
                if (csg is CsgBox3D box) {
                    // Use box Size directly, like BoxShape3D in collision shapes
                    var halfSize = box.Size / 2f;
                    foreach (var corner in BOX_CORNERS) {
                        var localPos = new Vector3(
                            corner.X * box.Size.X - halfSize.X,
                            corner.Y * box.Size.Y - halfSize.Y,
                            corner.Z * box.Size.Z - halfSize.Z
                        );
                        var worldPos = csg.GlobalTransform * localPos;
                        outMesh.Vertices.Add(ToLocalPoint3D(worldPos));
                    }
                    foreach (var index in BOX_INDICES) {
                        outMesh.Indices.Add(offset + index);
                    }
                    offset += BOX_CORNERS.Length;
                } else {
                    // Other CSG types: use AABB as box approximation
                    var localAabb = GetCsgLocalAabb(csg);
                    foreach (var corner in BOX_CORNERS) {
                        var localPos = new Vector3(
                            localAabb.Position.X + corner.X * localAabb.Size.X,
                            localAabb.Position.Y + corner.Y * localAabb.Size.Y,
                            localAabb.Position.Z + corner.Z * localAabb.Size.Z
                        );
                        var worldPos = csg.GlobalTransform * localPos;
                        outMesh.Vertices.Add(ToLocalPoint3D(worldPos));
                    }
                    foreach (var index in BOX_INDICES) {
                        outMesh.Indices.Add(offset + index);
                    }
                    offset += BOX_CORNERS.Length;
                }
            }
            return outMesh;
        }

        // --- MeshInstance3D → Polygon ---

        Geometry.Types.Polygon CreatePolygonFromMeshInstance(MeshInstance3D meshInstance) {
            var aabb = meshInstance.Mesh.GetAabb();
            return CreatePolygonFromAabb(aabb, meshInstance.GlobalTransform);
        }

        // --- Polygon from AABB helpers ---

        Geometry.Types.Polygon CreatePolygonFromAabb(Aabb aabb, Transform3D globalTransform) {
            var polygon = new Geometry.Types.Polygon();
            var corner1 = globalTransform * new Vector3(aabb.Position.X, 0, aabb.Position.Z);
            var corner2 = globalTransform * new Vector3(aabb.Position.X, 0, aabb.End.Z);
            var corner3 = globalTransform * new Vector3(aabb.End.X, 0, aabb.End.Z);
            var corner4 = globalTransform * new Vector3(aabb.End.X, 0, aabb.Position.Z);
            polygon.Points.Add(CreatePoint2D(corner1));
            polygon.Points.Add(CreatePoint2D(corner2));
            polygon.Points.Add(CreatePoint2D(corner3));
            polygon.Points.Add(CreatePoint2D(corner4));
            var worldMin = globalTransform * aabb.Position;
            var worldMax = globalTransform * aabb.End;
            polygon.Base = Mathf.Min(worldMin.Y, worldMax.Y);
            polygon.Height = Mathf.Max(worldMin.Y, worldMax.Y) - polygon.Base;
            return polygon;
        }

        Geometry.Types.Polygon CreatePolygonFromWorldAabb(Aabb bounds) {
            var polygon = new Geometry.Types.Polygon();
            var corner1 = new Vector3(bounds.Position.X, 0, bounds.Position.Z);
            var corner2 = new Vector3(bounds.Position.X, 0, bounds.End.Z);
            var corner3 = new Vector3(bounds.End.X, 0, bounds.End.Z);
            var corner4 = new Vector3(bounds.End.X, 0, bounds.Position.Z);
            polygon.Points.Add(CreatePoint2D(corner1));
            polygon.Points.Add(CreatePoint2D(corner2));
            polygon.Points.Add(CreatePoint2D(corner3));
            polygon.Points.Add(CreatePoint2D(corner4));
            polygon.Base = bounds.Position.Y;
            polygon.Height = bounds.Size.Y;
            return polygon;
        }

        // --- Point creation helpers ---

        Geometry.Types.Point2D CreatePoint2D(Vector3 position) {
            var point = new Geometry.Types.Point2D();
            if (useLocalCoordinates) {
                if (RTIPosition.LocalToRTIPosition != null) {
                    var rtiPos = RTIPosition.LocalToRTIPosition(position);
                    point.Local = new Geometry.Types.LocalPoint2D {
                        X = rtiPos.X,
                        Y = rtiPos.Z
                    };
                } else {
                    point.Local = new Geometry.Types.LocalPoint2D {
                        X = position.X,
                        Y = -position.Z
                    };
                }
            }
            if (useGeodeticCoordinates && RTIPosition.LocalToGeodetic != null) {
                var geodetic = RTIPosition.LocalToGeodetic(position);
                point.Geodetic = new Geometry.Types.GeodeticPoint2D {
                    Latitude = geodetic.Latitude,
                    Longitude = geodetic.Longitude
                };
            }
            return point;
        }

        Geometry.Types.Point2D CreatePoint2DFrom2D(Vector2 position) {
            return CreatePoint2D(new Vector3(position.X, 0, position.Y));
        }

        Geometry.Types.Point3D CreatePoint3D(Vector3 position) {
            var point = new Geometry.Types.Point3D();
            if (useLocalCoordinates) {
                if (RTIPosition.LocalToRTIPosition != null) {
                    var rtiPos = RTIPosition.LocalToRTIPosition(position);
                    point.Local = new Geometry.Types.LocalPoint3D {
                        X = rtiPos.X,
                        Y = rtiPos.Y,
                        Z = rtiPos.Z
                    };
                } else {
                    point.Local = new Geometry.Types.LocalPoint3D {
                        X = position.X,
                        Y = position.Y,
                        Z = -position.Z
                    };
                }
            }
            if (useGeodeticCoordinates && RTIPosition.LocalToGeodetic != null) {
                var geodetic = RTIPosition.LocalToGeodetic(position);
                point.Geodetic = new Geometry.Types.GeodeticPoint3D {
                    Latitude = geodetic.Latitude,
                    Longitude = geodetic.Longitude,
                    Altitude = geodetic.Altitude
                };
            }
            return point;
        }

        Geometry.Types.LocalPoint3D ToLocalPoint3D(Vector3 position) {
            if (RTIPosition.LocalToRTIPosition != null) {
                var rtiPos = RTIPosition.LocalToRTIPosition(position);
                return new Geometry.Types.LocalPoint3D {
                    X = rtiPos.X,
                    Y = rtiPos.Y,
                    Z = rtiPos.Z
                };
            }
            return new Geometry.Types.LocalPoint3D {
                X = position.X,
                Y = position.Y,
                Z = -position.Z
            };
        }

        // --- Color helpers ---

        static Inhumate.RTI.Proto.Color GetProtoColor(Godot.Color color) {
            if (color.A > 1e-5 || color.R > 1e-5 || color.G > 1e-5 || color.B > 1e-5) {
                return new Inhumate.RTI.Proto.Color {
                    Red = (int)Math.Round(color.R * 255),
                    Green = (int)Math.Round(color.G * 255),
                    Blue = (int)Math.Round(color.B * 255)
                };
            }
            return null;
        }

        // --- Box constants for collision mesh generation ---

        static readonly Vector3[] BOX_CORNERS = new Vector3[] {
            new Vector3(0, 0, 0),
            new Vector3(1, 0, 0),
            new Vector3(1, 1, 0),
            new Vector3(0, 1, 0),
            new Vector3(0, 1, 1),
            new Vector3(1, 1, 1),
            new Vector3(1, 0, 1),
            new Vector3(0, 0, 1),
        };

        static readonly int[] BOX_INDICES = new int[] {
            0, 2, 1, //face front
            0, 3, 2,
            2, 3, 4, //face top
            2, 4, 5,
            1, 2, 5, //face right
            1, 5, 6,
            0, 7, 4, //face left
            0, 4, 3,
            5, 4, 7, //face back
            5, 7, 6,
            0, 6, 7, //face bottom
            0, 1, 6
        };

    }

}
