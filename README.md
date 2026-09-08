# ECEN4610-Capstone-Lab-Adaptive-Wheelchair

Get files locally
  
        git clone <URL> 																			# Download repository locally
        git status 																						# See current state 
        git pull 																							# Get latest changes from github
        
   -- Branches  
Branch to create a new version of the entire repository. Work on new code here, merge with main later  
Name Branches based on the task you are working on
        
        git branch 																						# See branches 
        git branch feature/<task/branch name> 								# create branch without switching
        git checkout <branchname> 														# Switch branches 
        git checkout -b <branchname> 													# Create + switch branch 

Commit Changes to Github

        git add filename1.c filename2.c 											# Stage changes for multiple files
          git add foldername/ 																# Stage changes for entire folder
          git add . 																					# add everything in current directory
          
        git commit -m "message" 															# Save changes 
        
        git push																 							# Upload changes
          git push -u origin feature/<branchname> 						# first push of a new branch
          git push origin --delete feature/<branchname>			 	# deletes branch from github
          git branch -d feature/<branchname> 									# deletes branch locally
          
        git fetch 																						# Download remote information


  
