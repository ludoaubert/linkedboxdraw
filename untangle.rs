use itertools::Itertools;
use std::cmp::Ordering;
use std::collections::HashMap;
use serde::{Serialize, Deserialize};
use std::env;
use log::{debug};
use std::ops::Sub;
use std::f64::consts::PI;
use itertools::izip;
use std::collections::BTreeSet;

#[derive(Debug, PartialEq, Copy, Clone, Serialize, Deserialize, Eq, Ord, PartialOrd)]
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
#[derive(Deserialize)]
struct Link{
    from: u32,
    to: u32,
    polyline: Vec<Point>
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
#[derive(Debug, Clone, Copy, PartialEq, Serialize, Eq, Ord, PartialOrd)]
struct PointCoordinates {
    link_idx:usize,
    point_idx:usize
}
#[derive(Debug, PartialEq, Serialize, Eq, Ord, PartialOrd)]
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
    update:BTreeSet<BTreeSet<UpdateCommand>>
}
enum SegmentDirection
{
    Up,
    Down,
    Left,
    Right
}

fn rotate(p: &Point, angle: f64) -> Point {
    let (s, c) = angle.sin_cos();
    Point {
        x : ((p.x as f64) * c - (p.y as f64) * s).round() as i32,
        y : ((p.x as f64) * s + (p.y as f64) * c).round() as i32
    }
}

fn untangle(lnks:&Vec<Link>)->BTreeSet<BTreeSet<UpdateCommand>>{

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
    
    let update : BTreeSet<BTreeSet<UpdateCommand>> = links
        .iter()
        .chunk_by(|a| (a.from,a.edge))
        .into_iter()
        .map(|(key, group)| -> BTreeSet<UpdateCommand> {
            let (from,edge) = key;
            let lnks_ : Vec<ShallowLink> = group.cloned().collect();
                
            println!("{:?}", lnks_);
            
            let angle = match edge {
                RectangleEdge::Right => 0f64,
                RectangleEdge::Left => PI,
                RectangleEdge::Top => PI / 2.0,
                RectangleEdge::Bottom => -PI / 2.0           
            };
        
            println!("angle:{:?}", angle);

//This returns:

//Ordering::Less → segment goes upward
//Ordering::Greater → segment goes downward
//Ordering::Equal → horizontal (which you say cannot happen)
  
            let link_order = |i: &usize, j: &usize| -> Ordering {
                let a = &lnks_[*i].polyline;
                let b = &lnks_[*j].polyline;
    
                let ord = match (a.len(), b.len()) {
                    (2, 2) => rotate(a[0],angle).y.cmp(&rotate(b[0],angle).y),
    
                    (2, _) => rotate(b[1],angle).y.cmp(&rotate(b[2],angle).y),
    
                    (_, 2) => rotate(a[1],angle).y.cmp(&rotate(a[2],angle).y),
    
                    _ => {
                    
                        let segment_direction = |p: &[&Point]| {
                            if rotate(p[2],angle).y > rotate(p[1],angle).y {
                                SegmentDirection::Down
                            } else {
                                SegmentDirection::Up
                            }
                        };
                        
                        let dir_a = segment_direction(a);
                        let dir_b = segment_direction(b);
    
                        match (dir_a, dir_b) {
                            (SegmentDirection::Up, SegmentDirection::Up) => rotate(a[1],angle).x.cmp(&rotate(b[1],angle).x),
                            (SegmentDirection::Down, SegmentDirection::Down) => rotate(b[1],angle).x.cmp(&rotate(a[1],angle).x),
                            (SegmentDirection::Up, SegmentDirection::Down) => Ordering::Less,
                            (SegmentDirection::Down, SegmentDirection::Up) => Ordering::Greater,
                            _ => unreachable!("Polyline contains a non-axis-aligned segment")
                        }
                    }
                };
                
                ord
            };
            
            let n: usize = lnks_.len();
            
            let v: Vec<usize> = (0..n)
                .sorted_by(link_order)
                .collect();
       
            println!("{:?}", v);
            
            let vv: Vec<usize> = (0..n)
                .sorted_by(|i, j| {
                    let a = &lnks_[*i].polyline;
                    let b = &lnks_[*j].polyline;
            
                    let ord = rotate(a[0], angle).y.cmp(&rotate(b[0], angle).y);
                    ord
                })
                .collect();
        
            for i in 0..n {
                println!("{} {} {}", i, v[i], vv[i]);
            }
    
            let update:BTreeSet<UpdateCommand> = izip!(v.iter(), vv.iter())
                .map(|(x, i)|{
                    let tr : Point = lnks_[*i].polyline[0] - lnks_[*x].polyline[0];
                    (x, tr)
                })
                .filter(|(x,tr)|->bool {*tr != Point{x:0,y:0}})
                .map(|(x, tr)| {
                    let pc: Vec<PointCoordinates> = (0..2)
                        .map(|i| {
                            let p:&Point=lnks_[*x].polyline[i];
                            let key = p as *const Point;
                            let (link_idx, point_idx) = point_index[&key];
                            PointCoordinates{link_idx,point_idx}
                        })
                        .collect();
                    UpdateCommand{segment:(pc[0],pc[1]),translation:tr}
                })
                .collect();
    
            println!("{:?}", update);
            
            return update;
        })
        .filter(|update:&BTreeSet<UpdateCommand>| -> bool {update.is_empty()==false})
        .collect();
    
    return update;
}

fn main() {

    env_logger::init();

    let args: Vec<String> = env::args().collect();

    for arg in &args {
        println!("{arg}");
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
        let update = untangle(&lnks);
        let json = serde_json::to_string(&update).unwrap();
        println!("{}", json);
        return;
    }
    
    let test_contexts : [TestContext;4]=[
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
            update:BTreeSet::from([
                BTreeSet::from([UpdateCommand{segment:(PointCoordinates{link_idx:0,point_idx:0},PointCoordinates{link_idx:0,point_idx:1}),
                                translation:Point{x:0,y:20}},
                    UpdateCommand{segment:(PointCoordinates{link_idx:1,point_idx:0},PointCoordinates{link_idx:1,point_idx:1}),
                                translation:Point{x:0,y:-20}}]),
                BTreeSet::from([UpdateCommand{segment:(PointCoordinates{link_idx:0,point_idx:3},PointCoordinates{link_idx:0,point_idx:2}),
                                translation:Point{x:0,y:10}},
                    UpdateCommand{segment:(PointCoordinates{link_idx:1,point_idx:3},PointCoordinates{link_idx:1,point_idx:2}),
                                translation:Point{x:0,y:-10}}])
            ])

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
            update:BTreeSet::from([
                BTreeSet::from([UpdateCommand{segment:(PointCoordinates{link_idx:0,point_idx:0},PointCoordinates{link_idx:0,point_idx:1}),
                                    translation:Point{x:0,y:10}},
                    UpdateCommand{segment:(PointCoordinates{link_idx:1,point_idx:0},PointCoordinates{link_idx:1,point_idx:1}),
                                    translation:Point{x:0,y:-10}}
                ])
            ])
        },
        TestContext{
            lnks:vec![
                Link{from:0,to:2,polyline:vec![Point{x:40,y:50},Point{x:110,y:50},Point{x:110,y:120},Point{x:160,y:120}]},
                Link{from:0,to:1,polyline:vec![Point{x:40,y:70},Point{x:160,y:70}]}
            ],
/*
  10      40     110       160   190
  40+-----+
    |   50|-------+
    |  0  |       |         +-----+60
    |   70|-------+-------->|70   |
    |     |       |         |  1  |
  90+-----+       |         |     |
                  |         +-----+100
                  |         +-----+110
                  +-------->|120  |
                            |  2  |
                            |     |
                            +-----+150
*/
            update:BTreeSet::from([
                BTreeSet::from([UpdateCommand{segment:(PointCoordinates{link_idx:0,point_idx:0},PointCoordinates{link_idx:0,point_idx:1}),
                                    translation:Point{x:0,y:20}},
                    UpdateCommand{segment:(PointCoordinates{link_idx:1,point_idx:0},PointCoordinates{link_idx:1,point_idx:1}),
                                    translation:Point{x:0,y:-20}}
                ])
            ])
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
            update:BTreeSet::from([
                BTreeSet::from([UpdateCommand{segment:(PointCoordinates{link_idx:0,point_idx:0},PointCoordinates{link_idx:0,point_idx:1}),
                                translation:Point{x:0,y:20}},
                    UpdateCommand{segment:(PointCoordinates{link_idx:2,point_idx:0},PointCoordinates{link_idx:2,point_idx:1}),
                                translation:Point{x:0,y:-20}}]),
                BTreeSet::from([UpdateCommand{segment:(PointCoordinates{link_idx:0,point_idx:3},PointCoordinates{link_idx:0,point_idx:2}),
                                translation:Point{x:0,y:20}},
                    UpdateCommand{segment:(PointCoordinates{link_idx:2,point_idx:3},PointCoordinates{link_idx:2,point_idx:2}),
                                translation:Point{x:0,y:-20}}])
            ])
        }
    ];

    let angles:[f64;4]=[0f64, -PI, -PI / 2.0, PI / 2.0];
    
    let synthetic_test_contexts : Vec<TestContext> = test_contexts
        .iter()
        .cartesian_product(angles)
        .map(|(ctx,angle)| {
            TestContext{
                lnks:ctx.lnks
                    .iter()
                    .map(|lnk:&Link| Link{
                        from:lnk.from,
                        to:lnk.to,
                        polyline:lnk.polyline
                            .iter()
                            .map(|p| rotate(p, angle))
                            .collect(),
                    })
                    .collect(),
                update:ctx.update
                    .iter()
                    .map(|v:&BTreeSet<UpdateCommand>|
                            v
                            .iter()
                            .map(|uc:&UpdateCommand| UpdateCommand{
                                segment:uc.segment,
                                translation:rotate(&uc.translation,angle)
                            })
                            .collect()
                        )
                        .collect()
                }
            })
            .collect();
            
    let mut nbOK:u32=0;
    let mut nbKO:u32=0;

    for test_ctx in &synthetic_test_contexts {
        let update = untangle(&test_ctx.lnks);
        println!("{:?}", update);
        let json = serde_json::to_string(&update).unwrap();
        println!("{}", json);
        let b:bool = update==test_ctx.update;
        let status : &str = if b {"OK"} else {"KO"};
        println!("{}", status);
        if b{
            nbOK += 1;
        }else{
            nbKO += 1;
        }
    }
    println!("{}/{} tests OK.", nbOK, nbOK + nbKO);
}
