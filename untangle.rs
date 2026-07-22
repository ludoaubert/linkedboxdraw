use itertools::Itertools;
use std::cmp::Ordering;
use std::collections::HashMap;

#[derive(Debug, PartialEq, Copy, Clone)]
struct Point{
    x: i32,
    y: i32
}
#[derive(Debug, Copy, Clone)]
struct Coord {
    x: i32,
    y: i32,
}

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
#[derive(Debug, Clone, Copy)]
struct PointCoordinates {
    link_idx:usize,
    point_idx:usize
}
#[derive(Debug)]
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
    n:u32,
    lnks:Vec<Link>
}
enum SegmentDirection
{
    Up,
    Down,
    Left,
    Right
}

fn untangle(n:u32, lnks:&Vec<Link>)->Vec<Vec<UpdateCommand>>{

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
                
            println!("{:?}", lnks_);
            
            let mode:Orientation=match edge {
                RectangleEdge::Right => Orientation::Normal,
                RectangleEdge::Left => Orientation::ReverseXY,
                RectangleEdge::Top => Orientation::SwapXYReverseXY,
                RectangleEdge::Bottom => Orientation::SwapXY
            };
        
            println!("{:?}", mode);
            
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
                        let da = direction(a);
                        let db = direction(b);
                        
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
            let mut v: Vec<usize> = (0..n).collect();
       
            v.sort_by(link_order);
       
            println!("{:?}", v);
        
            for (i, x) in v.iter().enumerate() {
                println!("{} {}", i, x);
            }
        
            let result: Vec<_> = v
                .iter()
                .enumerate()
                .map(|(i, x)| lnks_[*x].polyline[0].y - lnks_[i].polyline[0].y)
                .collect();
    
            let update:Vec<UpdateCommand> = v
                .iter()
                .enumerate()
                .map(|(i, x)| (i, x, lnks_[*x].polyline[0].y - lnks_[i].polyline[0].y))
                .map(|(i, x, tr)| {
                    let pc: Vec<PointCoordinates> = (0..2)
                    .map(|j| {
                        let p:&Point=lnks_[i].polyline[j];
                        let key = p as *const Point;
                        let (link_idx, point_idx) = point_index[&key];
                        PointCoordinates{link_idx,point_idx}
                    })
                    .collect();
                    let (xtr, ytr) = match edge {
                        RectangleEdge::Right => (0,tr),
                        RectangleEdge::Left => (0,tr),
                        RectangleEdge::Top => (tr,0),
                        RectangleEdge::Bottom => (tr,0)
                    };
                    UpdateCommand{segment:(pc[0],pc[1]),translation:Point{x:xtr,y:ytr}}
                })
                .filter(|uc:&UpdateCommand| uc.translation!=Point{x:0,y:0})
                .collect();
    
            println!("{:?}", result);
            println!("{:?}", update);
            
            return update;
        })
        .collect();
    
    return updates;
}

fn main() {
    
    let test_contexts : [TestContext;3]=[
        TestContext{
            n:2,
            lnks:vec![
                Link{from:0,to:1,polyline:vec![Point{x:40,y:30},Point{x:90,y:30},Point{x:90,y:60},Point{x:180,y:60}]},
                Link{from:0,to:1,polyline:vec![Point{x:40,y:50},Point{x:110,y:50},Point{x:110,y:70},Point{x:180,y:70}]}
            ]
        },
        TestContext{
            n:3,
            lnks:vec![
                Link{from:0,to:2,polyline:vec![Point{x:40,y:50},Point{x:110,y:50},Point{x:110,y:120},Point{x:160,y:120}]},
                Link{from:0,to:1,polyline:vec![Point{x:40,y:60},Point{x:160,y:60}]}
            ]
        },
        TestContext{
            n:2,
            lnks:vec![
                Link{from:0,to:1,polyline:vec![Point{x:40,y:180},Point{x:100,y:180},Point{x:100,y:30},Point{x:120,y:30}]},
                Link{from:0,to:1,polyline:vec![Point{x:40,y:190},Point{x:90,y:190},Point{x:90,y:40},Point{x:120,y:40}]},
                Link{from:0,to:1,polyline:vec![Point{x:40,y:200},Point{x:80,y:200},Point{x:80,y:50},Point{x:120,y:50}]}
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
            n:ctx.n,
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
                        .collect()
                })
                .collect()
            })
            .collect();

    let test_ctx : &TestContext = &synthetic_test_contexts[0];
    let updates = untangle(test_ctx.n, &test_ctx.lnks);
}
