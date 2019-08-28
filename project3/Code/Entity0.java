public class Entity0 extends Entity
{
    // Perform any necessary initialization in the constructor
    public Entity0()
    {
        System.out.println("Initializing entity 0 at time: " + NetworkSimulator.time);
        // used to initialize distance table
        for(int x = 0; x < 4; x++)
        {
            for(int y = 0; y < 4; y++)
            {
                distanceTable[x][y] = 999;
            }
        }
        distanceTable[0][0] = 0;
        distanceTable[0][1] = 1;
        distanceTable[0][2] = 3;
        distanceTable[0][3] = 7;
        int i = 1;
        // send initial distance vectors to neighbors
        while(i < 4)
        {
            Packet packet = new Packet(0, i, distanceTable[0]);
            System.out.println("Sending first distance vector from 0 to " + i + " at " + NetworkSimulator.time);
            NetworkSimulator.toLayer2(packet);
            i++;
        }
        System.out.println();
    }

    // Handle updates when a packet is received.  You will need to call
    // NetworkSimulator.toLayer2() with new packets based upon what you
    // send to update.  Be careful to construct the source and destination of
    // the packet correctly.  Read the warning in NetworkSimulator.java for more
    // details.
    public void update(Packet p)
    {
        System.out.println("Packet received in 0 via " + p.getSource() + " at time: " + NetworkSimulator.time);
        int myDistanceVector [] = distanceTable[0];
        int neighbors [] = NetworkSimulator.cost[0];
        int i = 0;
        int oldDistanceVector[] = new int[4];
        while(i < 4)
        {
            oldDistanceVector[i] = myDistanceVector[i];
            i++;
        }
        i = 0;
        while(i < 4)
        {
            distanceTable[p.getSource()][i] = p.getMincost(i);
            i++;
        }
        i = 1;              // i = 1 as 0 does not need to check itself
        boolean vectorChanged = false;
        while(i < 4)
        {
            //int currentDistance = myDistanceVector[i];
            int directDistance = neighbors[i];
            int tempDistance = 999;
            int newDistance = 0;
            for(int x = 0; x <4; x++)
            {
                newDistance = distanceTable[x][i] + neighbors[x];
                if(newDistance < tempDistance)
                {
                    tempDistance = newDistance;
                }
            }
            if(directDistance < tempDistance)
            {
                myDistanceVector[i] = directDistance;
            }
            else
            {
                myDistanceVector[i] = tempDistance;
            }
            i++;
        }
        i = 0;
        while(i < 4)
        {
            if(myDistanceVector[i] != oldDistanceVector[i])
            {
                vectorChanged = true;
                break;
            }
            i++;
        }
        if(vectorChanged)
        {
            i = 0;
            while(i < 4)
            {
                distanceTable[0][i] = myDistanceVector[i];
                i++;
            }
            System.out.println("Distance vector changed in 0");
            toStringVector();
            printDT();
            // if the vector was changed, send it to neighbors
            i = 1;
            while(i < 4)
            {
                Packet packet = new Packet(0, i, distanceTable[0]);
                System.out.println("Sending distance vector from 0 to " + i + " at: " + NetworkSimulator.time);
                NetworkSimulator.toLayer2(packet);
                i++;
            }
        }
        else
        {
            System.out.println("The distance vector in 0 was not updated");
            printDT();
        }
        System.out.println();
    }

    public void linkCostChangeHandler(int whichLink, int newCost)
    {
        System.out.println("Link cost change detected in 0 for : Link " + whichLink +
                            " with newCost: " + newCost + " at time: " + NetworkSimulator.time);
        int myDistanceVector [] = distanceTable[0];
        int i = 0;
        boolean vectorChanged = false;
        int neighbors [] = NetworkSimulator.cost[0];
        int oldDistanceVector[] = new int[4];
        while(i < 4)
        {
            oldDistanceVector[i] = myDistanceVector[i];
            i++;
        }
        myDistanceVector[whichLink] = newCost;
        i = 0;
        while(i < 4)
        {
            //int currentDistance = myDistanceVector[i];
            int directDistance = neighbors[i];
            int tempDistance = 999;
            int newDistance = 0;
            for(int x = 0; x <4; x++)
            {
                if(x == 0)
                {
                    continue;
                }
                newDistance = distanceTable[x][i] + neighbors[x];
                if(newDistance < tempDistance)
                {
                    tempDistance = newDistance;
                }
            }
            if(directDistance < tempDistance)
            {
                myDistanceVector[i] = directDistance;
            }
            else
            {
                myDistanceVector[i] = tempDistance;
            }
            i++;
        }

        i = 0;
        while(i < 4)
        {
            if(myDistanceVector[i] != oldDistanceVector[i])
            {
                vectorChanged = true;
                break;
            }
            i++;
        }

        if(vectorChanged)
        {
            i = 0;
            while(i < 4)
            {
                distanceTable[0][i] = myDistanceVector[i];
                i++;
            }
            System.out.println("Distance vector changed in 0");
            toStringVector();
            printDT();
            // if the vector was changed, send it to neighbors
            i = 1;
            while(i < 4)
            {
                Packet packet = new Packet(0, i, distanceTable[0]);
                System.out.println("Sending distance vector from 0 to " + i + " at: " + NetworkSimulator.time);
                NetworkSimulator.toLayer2(packet);
                i++;
            }
        }
        else
        {
            System.out.println("The distance vector in 0 was not updated");
            printDT();
        }
        System.out.println();
    }

    public void printDT()
    {
        System.out.println();
        System.out.println("           via");
        System.out.println(" D0 |   1   2   3");
        System.out.println("----+------------");
        for (int i = 1; i < NetworkSimulator.NUMENTITIES; i++)
        {
            System.out.print("   " + i + "|");
            for (int j = 1; j < NetworkSimulator.NUMENTITIES; j++)
            {
                if (distanceTable[i][j] < 10)
                {
                    System.out.print("   ");
                }
                else if (distanceTable[i][j] < 100)
                {
                    System.out.print("  ");
                }
                else
                {
                    System.out.print(" ");
                }

                System.out.print(distanceTable[i][j]);
            }
            System.out.println();
        }
    }
    public void toStringVector()
    {
        System.out.println("Updated distance vector for 0:");
        System.out.println("[" + distanceTable[0][0] + ", " + distanceTable[0][1] + ", " + distanceTable[0][2] + ", " + distanceTable[0][3] + "]");
    }
}
