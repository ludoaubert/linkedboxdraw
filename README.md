# linkedboxdraw

Edition of relational diagrams.

https://ludoaubert.github.io/linkedboxdraw/tableinput.html

Is a simple GUI where you can type in your box (table) names, and a list of fields (columns) that belong to each box. You can also create logical links from one box to another (foreign keys). Sometimes, certain boxes are connected to almost all other boxes, in which case you might want to express this logical link using colors instead of geometry.
You can save your diagram data to a json file. You can also compute and save a geometric information file. The process to compute this geometric information is :
1) break the diagram down into clusters, with a maximum of 20 boxes per cluster, cutting as few links as possible.
2) compute the translation of each box in its cluster, in such a way that two boxes that are connected should possibly be close to each other.
3) compute the geometric links materialized by New York City Street network like polylines (interconnected North South or East West segments). Polylines should be as short as possible, make as few turns as possible, cross each other as little as possible. 

This geometric information file contains box translations and polylines and has a json format (although a .js extension).

https://ludoaubert.github.io/linkedboxdraw/connected_rectangles.html

Is where you can view the diagram itself.
