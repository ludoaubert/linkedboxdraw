use itertools::Itertools;
use std::cmp::Ordering;
use std::collections::HashMap;
use serde::{Serialize, Deserialize};
use std::env;
use log::{debug};
use std::ops::Sub;

#[derive(Debug, PartialEq, Copy, Clone, Serialize, Deserialize)]
struct Point{
    x: i32,
    y: i32
}
impl Sub for Point {
    type Output = Point;

    fn sub(self, rhs: Point) -> Point {
        Point {
            x: self.x - rhs.x,
            y: self.y - rhs.y,
        }
    }
}
impl Sub<&Point> for &Point {
    type Output = Point;

    fn sub(self, rhs: &Point) -> Point {
        Point {
            x: self.x - rhs.x,
            y: self.y - rhs.y,
        }
    }
}
#[derive(Debug, Copy, Clone)]
struct Coord {
    x: i32,
    y: i32,
}
#[derive(Deserialize)]
struct Link{
    from: u32,
    to: u32,
    polyline: Vec<Point>
}
#[derive(Debug, Copy, Clone)]
enum Orientation {
    Normal,
    ReverseXY,
    SwapXY,
    SwapXYReverseXY
}
#[derive(Eq, Ord, Debug, PartialEq, PartialOrd, Copy, Clone)]
enum RectangleEdge {
    Left,
    Right,
    Top,
    Bottom
}
#[derive(Debug, PartialEq, Copy, Clone)]
enum PolylineDirection {
    Forward,
    Backward
}
#[derive(Debug, Clone, Copy, Serialize)]
struct PointCoordinates {
    link_idx:usize,
    point_idx:usize
}
#[derive(Debug, Serialize)]
struct UpdateCommand {
    segment:(PointCoordinates, PointCoordinates),
    translation:Point
}
#[derive(Debug, Clone)]
struct ShallowLink<'a>{
    direction: PolylineDirection,
    from:u32,
    edge:RectangleEdge,
    to:u32,
    polyline:Vec<&'a Point>
}
struct TestContext{
    lnks:Vec<Link>,
    update:Vec<Vec<UpdateCommand>>
}
enum SegmentDirection
{
    Up,
    Down,
    Left,
    Right
}

fn untangle(lnks:&Vec<Link>)->Vec<Vec<UpdateCommand>>{

    let point_index: HashMap<*const Point, (usize, usize)> = lnks
        .iter()
        .enumerate()
        .flat_map(|(link_idx, lnk)| {
            lnk.polyline
                .iter()
                .enumerate()
                .map(move |(point_idx, p)| {
                    (p as *const Point, (link_idx, point_idx))
                })
        })
        .collect();
    
    let links : Vec<ShallowLink> = lnks
        .iter()
        .cartesian_product([PolylineDirection::Forward,PolylineDirection::Backward])
        .map(|(lnk,dir)| -> (u32,u32,Vec<&Point>,PolylineDirection) {
            match dir {
                PolylineDirection::Forward => (lnk.from, lnk.to, lnk.polyline.iter().collect(), dir),
                PolylineDirection::Backward => (lnk.to, lnk.from, lnk.polyline.iter().rev().collect(), dir)
            }
        })
        .map(|(from,to,p,dir)| -> ShallowLink {
            let edge = match (p[0].x.cmp(&p[1].x), p[0].y.cmp(&p[1].y)) {
                (Ordering::Equal, Ordering::Greater) => RectangleEdge::Top,
                (Ordering::Equal, Ordering::Less)    => RectangleEdge::Bottom,
                (Ordering::Greater, Ordering::Equal) => RectangleEdge::Left,
                (Ordering::Less, Ordering::Equal)    => RectangleEdge::Right,
                _ => unreachable!("Polyline contains a non-axis-aligned segment")
            };
            ShallowLink{direction:dir,from:from,edge:edge,to:to,polyline:p}
        })
        .sorted_by(|a, b| (a.from,a.edge).cmp(&(b.from,b.edge)))
        .collect();
    
    let updates : Vec<Vec<UpdateCommand>> = links
        .iter()
        .chunk_by(|a| (a.from,a.edge))
        .into_iter() // converts ChunkBy into an Iterator
        .map(|(key, group)| -> Vec<UpdateCommand> {
            let (from,edge) = key;
            let lnks_ : Vec<ShallowLink> = group.cloned().collect();
                
            debug!("{:?}", lnks_);
            
            let mode:Orientation=match edge {
                RectangleEdge::Right => Orientation::Normal,
                RectangleEdge::Left => Orientation::ReverseXY,
                RectangleEdge::Top => Orientation::SwapXYReverseXY,
                RectangleEdge::Bottom => Orientation::SwapXY
            };
        
            debug!("{:?}", mode);
            
            let transform_point = |p: &Point| -> Coord {
                match mode {
                    Orientation::Normal => Coord { x: p.x,  y: p.y },
                    Orientation::ReverseXY => Coord { x: -p.x, y: -p.y },
                    Orientation::SwapXY => Coord { x: -p.y,  y: p.x },
                    Orientation::SwapXYReverseXY => Coord { x: p.y, y: -p.x }
                }
            };
       
            let direction = |p: &[&Point]| -> Ordering {
                transform_point(p[2]).y.cmp(&transform_point(p[1]).y)
            };

//This returns:

//Ordering::Less → segment goes upward
//Ordering::Greater → segment goes downward
//Ordering::Equal → horizontal (which you say cannot happen)
  
            let link_order = |i: &usize, j: &usize| -> Ordering {
                let a = &lnks_[*i].polyline;
                let b = &lnks_[*j].polyline;
    
                let order = match (a.len(), b.len()) {
                    (2, 2) => transform_point(a[0]).y.cmp(&transform_point(b[0]).y),
    
                    (2, _) => Ordering::Equal.cmp(&direction(b)),
    
                    (_, 2) => Ordering::Equal.cmp(&direction(a)),
    
                    _ => {
                    
                        let segment_direction = |p: &[&Point]| {
                            if transform_point(p[2]).y > transform_point(p[1]).y {
                                SegmentDirection::Down
                            } else {
                                SegmentDirection::Up
                            }
                        };
                        
                        let dir_a = segment_direction(a);
                        let dir_b = segment_direction(b);
    
                        match (dir_a, dir_b) {
                            (SegmentDirection::Up, SegmentDirection::Up) => transform_point(a[1]).x.cmp(&transform_point(b[1]).x),
                            (SegmentDirection::Down, SegmentDirection::Down) => transform_point(b[1]).x.cmp(&transform_point(a[1]).x),
                            (SegmentDirection::Up, SegmentDirection::Down) => Ordering::Less,
                            (SegmentDirection::Down, SegmentDirection::Up) => Ordering::Greater,
                            _ => unreachable!("Polyline contains a non-axis-aligned segment")
                        }
                    }
                };
                
                match mode {
                    Orientation::Normal => order,
                    Orientation::ReverseXY => order.reverse(),
                    Orientation::SwapXY => order,
                    Orientation::SwapXYReverseXY => order.reverse()    
                }
            };
            
            let n: usize = lnks_.len();
            
            let v: Vec<usize> = (0..n)
                .sorted_by(link_order)
                .collect();
       
            debug!("{:?}", v);
            
            let vv: Vec<usize> = (0..n)
                .sorted_by(|i: &usize, j: &usize| -> Ordering {
                    let a = &lnks_[*i].polyline;
                    let b = &lnks_[*j].polyline;
                    let order = transform_point(b[0]).y.cmp(&transform_point(a[0]).y);
                    
                    match mode {
                        Orientation::Normal => order,
                        Orientation::ReverseXY => order.reverse(),
                        Orientation::SwapXY => order,
                        Orientation::SwapXYReverseXY => order.reverse()    
                    }
                })
                .collect();
        
            for i in 0..n {
                debug!("{} {}", v[i], vv[i]);
            }
    
            let update:Vec<UpdateCommand> = (0..n)
                .enumerate()
                .map(|(i, x)| {
                    let tr : Point = lnks_[vv[i]].polyline[0] - lnks_[x].polyline[0];
                    let pc: Vec<PointCoordinates> = (0..2)
                        .map(|j| {
                            let p:&Point=lnks_[x].polyline[j];
                            let key = p as *const Point;
                            let (link_idx, point_idx) = point_index[&key];
                            PointCoordinates{link_idx,point_idx}
                        })
                        .collect();
                    UpdateCommand{segment:(pc[0],pc[1]),translation:tr}
                })
                .filter(|uc:&UpdateCommand| uc.translation!=Point{x:0,y:0})
                .collect();
    
            debug!("{:?}", update);
            
            return update;
        })
        .collect();
    
    return updates;
}

fn main() {

    env_logger::init();

    let args: Vec<String> = env::args().collect();

    for arg in &args {
        debug!("{arg}");
    }
    
    if args.len()==2
    {
        let lnks: Vec<Link> = match serde_json::from_str(&args[1]) {
            Ok(v) => v,
            Err(e) => {
                eprintln!("Invalid JSON: {}", e);
                return;
            }
        };
        let updates = untangle(&lnks);
        let json = serde_json::to_string(&updates).unwrap();
        println!("{}", json);
        return;
    }
    
    let test_contexts : [TestContext;3]=[
        TestContext{
            lnks:vec![
                Link{from:0,to:1,polyline:vec![Point{x:40,y:30},Point{x:90,y:30},Point{x:90,y:60},Point{x:180,y:60}]},
                Link{from:0,to:1,polyline:vec![Point{x:40,y:50},Point{x:110,y:50},Point{x:110,y:70},Point{x:180,y:70}]}
            ],
/*
         40      90 110    180
    +-----+
    |   30|-------+         +-----+
    |     |       |         |     |
    |   50|-------+--+      |  1  |
    |  0  |       +--+----->|60   |
    +-----+          +----->|70   |
                            +-----+
*/
            update:vec![
                vec![UpdateCommand{
                    segment:(PointCoordinates{link_idx:0,point_idx:0},PointCoordinates{link_idx:0,point_idx:1}),
                    translation:Point{x:0,y:20}}, 
                    UpdateCommand{
                    segment:(PointCoordinates{link_idx:1,point_idx:0},PointCoordinates{link_idx:1,point_idx:1}),
                    translation:Point{x:0,y:-20}}
                ], 
                vec![UpdateCommand{
                    segment:(PointCoordinates{link_idx:0,point_idx:3},PointCoordinates{link_idx:0,point_idx:2}),
                    translation:Point{x:0,y:10}},
                    UpdateCommand{
                    segment:(PointCoordinates{link_idx:1,point_idx:3},PointCoordinates{link_idx:1,point_idx:2}),
                    translation:Point{x:0,y:-10}}
                ]
            ]
        },
        TestContext{
            lnks:vec![
                Link{from:0,to:2,polyline:vec![Point{x:40,y:50},Point{x:110,y:50},Point{x:110,y:120},Point{x:160,y:120}]},
                Link{from:0,to:1,polyline:vec![Point{x:40,y:60},Point{x:160,y:60}]}
            ],
/*               110       160
         40                 +-----+
    +-----+                 |     |
    |   50|-------+         |  1  |
    |   60|-------+-------->|     |
    |  0  |       |         +-----+
    +-----+       |         +-----+
                  |         |     |
                  +-------->|120  |
                            |  2  |
                            +-----+
*/
            update:vec![
                vec![UpdateCommand{segment:(PointCoordinates{link_idx:0,point_idx:0},PointCoordinates{link_idx:0,point_idx:1}),
                    translation:Point{x:0,y:10}},
                    UpdateCommand{segment:(PointCoordinates{link_idx:1,point_idx:0},PointCoordinates{link_idx:1,point_idx:1}),
                    translation:Point{x:0,y:-10}}
                ],
                vec![],
                vec![]
                ]
        },
        TestContext{
            lnks:vec![
                Link{from:0,to:1,polyline:vec![Point{x:40,y:180},Point{x:100,y:180},Point{x:100,y:30},Point{x:120,y:30}]},
                Link{from:0,to:1,polyline:vec![Point{x:40,y:190},Point{x:90,y:190},Point{x:90,y:40},Point{x:120,y:40}]},
                Link{from:0,to:1,polyline:vec![Point{x:40,y:200},Point{x:80,y:200},Point{x:80,y:50},Point{x:120,y:50}]}
            ],
/*
                              +-----+
                        +---->|30   |
                     +--+---->|40   |
                  +--+--+---->|50   |
                  |  |  |     |  1  |
   10    40       |  |  |     +-----+
    +-----+       |  |  |
    |  180|-------+--+--+
    |  190|-------+--+      
    |  200|-------+
    |  0  |
    +-----+      80 90 100
*/
            update:vec![
                vec![UpdateCommand{segment:(PointCoordinates{link_idx:0,point_idx:0},PointCoordinates{link_idx:0,point_idx:1}),
                    translation:Point{x:0,y:20}},
                    UpdateCommand{segment:(PointCoordinates{link_idx:2,point_idx:0},PointCoordinates{link_idx:2,point_idx:1}),
                    translation:Point{x:0,y:-20}}
                    ],
                vec![UpdateCommand{segment:(PointCoordinates{link_idx:0,point_idx:3},PointCoordinates{link_idx:0,point_idx:2}),
                    translation:Point{x:0,y:20}},
                    UpdateCommand{segment:(PointCoordinates{link_idx:2,point_idx:3},PointCoordinates{link_idx:2,point_idx:2}),
                    translation:Point{x:0,y:-20}}
                    ]
                ]
            ]
        }
    ];
    
    let modes:[Orientation;4]=[
        Orientation::Normal,
        Orientation::ReverseXY,
        Orientation::SwapXY,
        Orientation::SwapXYReverseXY             
    ];
    
    let synthetic_test_contexts : Vec<TestContext> = test_contexts
        .iter()
        .cartesian_product(modes)
        .map(|(ctx,mode)| TestContext{
            lnks:ctx.lnks
                .iter()
                .map(|lnk:&Link| Link{
                    from:lnk.from,
                    to:lnk.to,
                    polyline:lnk.polyline
                        .iter()
                        .map(|p:&Point|->Point{
                            match mode {
                                Orientation::Normal => Point{x:p.x,y:p.y},
                                Orientation::ReverseXY => Point{x:-p.x,y:-p.y},
                                Orientation::SwapXY => Point{x:p.y,y:-p.x},
                                Orientation::SwapXYReverseXY => Point{x:-p.y,y:p.x}     
                            }
                        })
                        .collect(),
                })
                .collect(),
                update:vec![]
            })
            .collect();

    let test_ctx : &TestContext = &synthetic_test_contexts[2*4];
    let updates = untangle(&test_ctx.lnks);
    println!("{:?}", updates);
    let json = serde_json::to_string(&updates).unwrap();
    println!("{}", json);
}
